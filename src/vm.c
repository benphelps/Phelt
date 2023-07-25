#include <libgen.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "compiler.h"
#include "debug.h"
#include "ph_string.h"
#include "vm.h"

VM vm;

static void resetStack(void)
{
    vm.stackTop     = vm.stack;
    vm.frameCount   = 0;
    vm.openUpvalues = NULL;
    vm.errorState   = false;
}

void runtimeError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame*   frame       = &vm.frames[i];
        ObjFunction* function    = frame->closure->function;
        size_t       instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
            (int)AS_NUMBER(function->chunk.lines.values[instruction]));
        if (function->name == NULL) {
            fprintf(stderr, "%s\n", basename((char*)function->source));
        } else {
            fprintf(stderr, "%s\n", function->name->chars);
        }
    }

    resetStack();
    vm.errorState = true;
}

void defineNative(Table* dest, const char* name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(dest, vm.stack[0], vm.stack[1]);
    pop();
    pop();
}

static void initNative(void)
{
    for (NativeFnEntry* entry = globalFns; entry->name != NULL; entry++) {
        defineNative(&vm.globals, entry->name, entry->function);
    }
}

void initVM(void)
{
    resetStack();
    vm.objects = NULL;

    vm.bytesAllocated = 0;
    vm.nextGC         = 1024 * 1024;
    vm.grayCount      = 0;
    vm.grayCapacity   = 0;
    vm.grayStack      = NULL;
    vm.errorState     = false;

    initTable(&vm.globals);
    initTable(&vm.strings);

    vm.initString   = NULL;
    vm.initString   = copyString("init", 4);
    vm.strString    = NULL;
    vm.strString    = copyString("__str", 5);
    vm.addString    = NULL;
    vm.addString    = copyString("__add", 5);
    vm.subString    = NULL;
    vm.subString    = copyString("__sub", 5);
    vm.mulString    = NULL;
    vm.mulString    = copyString("__mul", 5);
    vm.divString    = NULL;
    vm.divString    = copyString("__div", 5);
    vm.gtString     = NULL;
    vm.gtString     = copyString("__gt", 4);
    vm.gteString    = NULL;
    vm.gteString    = copyString("__gte", 4);
    vm.ltString     = NULL;
    vm.ltString     = copyString("__lt", 4);
    vm.lteString    = NULL;
    vm.lteString    = copyString("__lte", 4);
    vm.eqString     = NULL;
    vm.eqString     = copyString("__eq", 4);
    vm.neqString    = NULL;
    vm.neqString    = copyString("__neq", 4);
    vm.andString    = NULL;
    vm.andString    = copyString("__and", 5);
    vm.orString     = NULL;
    vm.orString     = copyString("__or", 4);
    vm.xorString    = NULL;
    vm.xorString    = copyString("__xor", 5);
    vm.modString    = NULL;
    vm.modString    = copyString("__mod", 5);
    vm.notString    = NULL;
    vm.notString    = copyString("__not", 5);
    vm.rshiftString = NULL;
    vm.rshiftString = copyString("__rshift", 8);
    vm.lshiftString = NULL;
    vm.lshiftString = copyString("__lshift", 8);

    initNative();
}

void freeVM(void)
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString   = NULL;
    vm.strString    = NULL;
    vm.addString    = NULL;
    vm.subString    = NULL;
    vm.mulString    = NULL;
    vm.divString    = NULL;
    vm.gtString     = NULL;
    vm.gteString    = NULL;
    vm.ltString     = NULL;
    vm.lteString    = NULL;
    vm.eqString     = NULL;
    vm.neqString    = NULL;
    vm.andString    = NULL;
    vm.orString     = NULL;
    vm.xorString    = NULL;
    vm.modString    = NULL;
    vm.notString    = NULL;
    vm.rshiftString = NULL;
    vm.lshiftString = NULL;
    freeObjects();
}

__attribute__((always_inline)) inline void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

__attribute__((always_inline)) inline Value pop(void)
{
    vm.stackTop--;
    return *vm.stackTop;
}

__attribute__((always_inline)) inline static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

bool call(ObjClosure* closure, int argCount)
{
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    if (vm.errorState) {
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure   = closure;
    frame->ip        = closure->function->chunk.code;
    frame->slots     = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound      = AS_BOUND_METHOD(callee);
            vm.stackTop[-argCount - 1] = bound->receiver;
            return call(bound->method, argCount);
        }
        case OBJ_CLASS: {
            ObjClass* klass            = AS_CLASS(callee);
            vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

            Value initializer;
            if (tableGet(&klass->methods, OBJ_VAL(vm.initString), &initializer)) {
                return call(AS_CLOSURE(initializer), argCount);
            } else if (argCount != 0) {
                runtimeError("Expected 0 arguments but got %d.", argCount);
                return false;
            }

            return true;
        }
        case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), argCount);
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            if (native(argCount, vm.stackTop - argCount)) {
                vm.stackTop -= argCount;
                return true;
            } else {
                runtimeError(AS_STRING(vm.stackTop[-argCount - 1])->chars);
                return false;
            }
        }
        default:
            break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool invokeFromClass(ObjClass* klass, Value name, int argCount)
{
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("1Undefined property '%s'.", stringValue(name));
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

static bool invoke(Value name, int argCount)
{
    Value receiver = peek(argCount);

    switch (OBJ_TYPE(receiver)) {
    case OBJ_INSTANCE: {
        ObjInstance* instance = AS_INSTANCE(receiver);

        Value value;
        if (tableGet(&instance->fields, name, &value)) {
            vm.stackTop[-argCount - 1] = value;
            return callValue(value, argCount);
        }

        return invokeFromClass(instance->klass, name, argCount);
    }
    case OBJ_TABLE: {
        ObjTable* table = AS_TABLE(receiver);
        Value     value;
        if (tableGet(&table->table, name, &value)) {
            vm.stackTop[-argCount - 1] = value;
            return callValue(value, argCount);
        }
        runtimeError("Undefined property '%s'.", stringValue(name));
        return false;
    }
    default:
        break; // Non-callable object type.
    };

    runtimeError("Only instances have methods.");
    return false;
}

static bool bindMethod(ObjClass* klass, Value name)
{
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", stringValue(name));
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

bool valueSlice(Value start, Value end)
{
    Value value = pop();

    if (IS_OBJ(value)) {
        switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            int i = (int)AS_NUMBER(start);
            int j = (int)AS_NUMBER(end);

            ObjString* string     = AS_STRING(value);
            int        str_length = utf8len(string->chars);

            if (j < 0)
                j = (str_length) + j;

            if (i < 0)
                i = (str_length) + i;

            if (i < 0 || i >= str_length || j < 0 || j >= str_length) {
                runtimeError("String index out of bounds.");
                return false;
            }

            char* substring = substring_utf8(string->chars, i, j);
            push(OBJ_VAL(takeString(substring, strlen(substring))));
            return true;
            break;
        }
        case OBJ_ARRAY: {
            ObjArray* array = AS_ARRAY(value);

            unsigned int i = (unsigned int)AS_NUMBER(start); // 2
            unsigned int j = (unsigned int)AS_NUMBER(end);   // 5

            if (j < 0)
                j = array->array.count + j;

            if (i < 0)
                i = array->array.count + i;

            if (i < 0 || i >= array->array.count || j < 0 || j >= array->array.count) {
                runtimeError("Array index out of bounds.");
                return false;
            }
            ObjArray* new = newArray();
            copyValueArray(&array->array, &new->array, i, j);
            push(OBJ_VAL(new));
            return true;
            break;
        }
        default:
            break;
        }
    }
    runtimeError("Only strings and arrays can be sliced.");
    return false;
}

static ObjUpvalue* captureUpvalue(Value* local)
{
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue     = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue     = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next       = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value* last)
{
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed     = *upvalue->location;
        upvalue->location   = &upvalue->closed;
        vm.openUpvalues     = upvalue->next;
    }
}

static void defineMethod(ObjString* name)
{
    Value     method = peek(0);
    ObjClass* klass  = AS_CLASS(peek(1));
    tableSet(&klass->methods, OBJ_VAL(name), method);
    pop();
}

__attribute__((always_inline)) inline static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(void)
{
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int   length = a->length + b->length;
    char* chars  = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

InterpretResult run(void)
{
    register CallFrame*   frame;
    register Value*       stackStart;
    register uint8_t*     ip;
    register ObjFunction* fn;

    if (vm.errorState) {
        return INTERPRET_RUNTIME_ERROR;
    }

#define LOAD_FRAME()                            \
    frame      = &vm.frames[vm.frameCount - 1]; \
    stackStart = frame->slots;                  \
    ip         = frame->ip;                     \
    fn         = frame->closure->function;

#define STORE_FRAME() frame->ip = ip

#define PUSH(value) (*vm.stackTop++ = value)
#define POP() (*(--vm.stackTop))
#define DROP() (--vm.stackTop)
#define PEEK() (*(vm.stackTop - 1))
#define PEEK2() (*(vm.stackTop - 2))
#define PEEK3() (*(vm.stackTop - 3))
#define PEEK4() (*(vm.stackTop - 4))
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

#define READ_CONSTANT() (fn->chunk.constants.values[READ_SHORT()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                         \
    do {                                                 \
        if (!IS_NUMBER(PEEK()) || !IS_NUMBER(PEEK2())) { \
            STORE_FRAME();                               \
            runtimeError("Operands must be numbers.");   \
            return INTERPRET_RUNTIME_ERROR;              \
        }                                                \
        double b = AS_NUMBER(POP());                     \
        double a = AS_NUMBER(POP());                     \
        PUSH(valueType(a op b));                         \
    } while (false)

#define BINARY_OP_INT(valueType, op)                     \
    do {                                                 \
        if (!IS_NUMBER(PEEK()) || !IS_NUMBER(PEEK2())) { \
            STORE_FRAME();                               \
            runtimeError("Operands must be numbers.");   \
            return INTERPRET_RUNTIME_ERROR;              \
        }                                                \
        int b = (int)AS_NUMBER(POP());                   \
        int a = (int)AS_NUMBER(POP());                   \
        PUSH(valueType(a op b));                         \
    } while (false)

#define INVOKE_DUNDER(dunderMethod)                                            \
    Obj* this  = AS_OBJ(PEEK2());                                              \
    Obj* other = AS_OBJ(PEEK());                                               \
    if (this->type == OBJ_INSTANCE && other->type == OBJ_INSTANCE) {           \
        ObjInstance* thisInstance  = (ObjInstance*)this;                       \
        ObjInstance* otherInstance = (ObjInstance*)other;                      \
        if (thisInstance->klass != otherInstance->klass) {                     \
            STORE_FRAME();                                                     \
            runtimeError("Operands must be two instances of the same class."); \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        Value method   = OBJ_VAL(dunderMethod);                                \
        int   argCount = 1;                                                    \
        STORE_FRAME();                                                         \
        if (!invoke(method, argCount)) {                                       \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        LOAD_FRAME();                                                          \
    }

#ifdef DEBUG_TRACE_EXECUTION
#define TRACE_EXECUTION()                                              \
    do {                                                               \
        if (!vm.errorState) {                                          \
            if (vm.stackTop != vm.stack) {                             \
                printf("           ");                                 \
            }                                                          \
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) { \
                printf("[ ");                                          \
                printValue(*slot);                                     \
                printf(" ]");                                          \
            }                                                          \
            printf("\n");                                              \
            disassembleInstruction(                                    \
                &fn->chunk,                                            \
                (int)(ip - fn->chunk.code),                            \
                false);                                                \
        }                                                              \
    } while (false)
#else
#define TRACE_EXECUTION() \
    do {                  \
    } while (false)
#endif

#ifdef COMPUTED_GOTO
    static void* dispatchTable[] = {
#define OPCODE(op) &&code_##op,
#include "opcodes.h"
#undef OPCODE
    };

#define INTERPRET_LOOP DISPATCH();
#define CASE_CODE(name) code_##name
#define DISPATCH()                                              \
    TRACE_EXECUTION();                                          \
    if (!vm.errorState) {                                       \
        goto* dispatchTable[instruction = (OpCode)READ_BYTE()]; \
    } else {                                                    \
        return INTERPRET_RUNTIME_ERROR;                         \
    }

#else
#define INTERPRET_LOOP \
    loop:              \
    TRACE_EXECUTION(); \
    switch (instruction = (OpCode)READ_BYTE())

#define CASE_CODE(name) case OP_##name
#define DISPATCH()                      \
    if (!vm.errorState) {               \
        goto loop;                      \
    } else {                            \
        return INTERPRET_RUNTIME_ERROR; \
    }

#endif

    LOAD_FRAME();

    OpCode instruction;
    INTERPRET_LOOP
    {
        CASE_CODE(CONSTANT)
            :
        {
            Value constant = READ_CONSTANT();
            PUSH(constant);
            DISPATCH();
        }

        CASE_CODE(NIL)
            :
        {
            PUSH(NIL_VAL);
            DISPATCH();
        }

        CASE_CODE(TRUE)
            :
        {
            PUSH(BOOL_VAL(true));
            DISPATCH();
        }

        CASE_CODE(FALSE)
            :
        {
            PUSH(BOOL_VAL(false));
            DISPATCH();
        }

        CASE_CODE(GET_UPVALUE)
            :
        {
            uint16_t slot = READ_SHORT();
            PUSH(*frame->closure->upvalues[slot]->location);
            DISPATCH();
        }

        CASE_CODE(SET_UPVALUE)
            :
        {
            uint16_t slot                             = READ_SHORT();
            *frame->closure->upvalues[slot]->location = PEEK();
            DISPATCH();
        }

        CASE_CODE(NOT_EQUAL)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.neqString);
            } else if (IS_ARRAY(PEEK()) && IS_ARRAY(PEEK2())) {
                ObjArray* b = AS_ARRAY(POP());
                ObjArray* a = AS_ARRAY(POP());
                PUSH(BOOL_VAL(!arraysEqual(&a->array, &b->array)));
            } else {
                Value b = POP();
                Value a = POP();
                PUSH(BOOL_VAL(!valuesEqual(a, b)));
            }
            DISPATCH();
        }

        CASE_CODE(EQUAL)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.eqString);
            } else if (IS_ARRAY(PEEK()) && IS_ARRAY(PEEK2())) {
                ObjArray* b = AS_ARRAY(POP());
                ObjArray* a = AS_ARRAY(POP());
                PUSH(BOOL_VAL(arraysEqual(&a->array, &b->array)));
            } else {
                Value b = POP();
                Value a = POP();
                PUSH(BOOL_VAL(valuesEqual(a, b)));
            }
            DISPATCH();
        }

        CASE_CODE(GREATER)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.gtString);
            } else {
                BINARY_OP(BOOL_VAL, >);
            }
            DISPATCH();
        }

        CASE_CODE(GREATER_EQUAL)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.gteString);
            } else {
                BINARY_OP(BOOL_VAL, >=);
            }
            DISPATCH();
        }

        CASE_CODE(LESS)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.ltString);
            } else {
                BINARY_OP(BOOL_VAL, <);
            }
            DISPATCH();
        }

        CASE_CODE(LESS_EQUAL)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.lteString);
            } else {
                BINARY_OP(BOOL_VAL, <=);
            }
            DISPATCH();
        }

        CASE_CODE(ADD)
            :
        {
            if (IS_STRING(PEEK()) && IS_STRING(PEEK2())) {
                concatenate();
            } else if (IS_NUMBER(PEEK()) && IS_NUMBER(PEEK2())) {
                double b = AS_NUMBER(POP());
                double a = AS_NUMBER(POP());
                PUSH(NUMBER_VAL(a + b));
            } else if (IS_TABLE(PEEK()) && IS_TABLE(PEEK2())) {
                ObjTable* b   = AS_TABLE(POP());
                ObjTable* a   = AS_TABLE(POP());
                ObjTable* new = newTable();
                tableAddAll(&b->table, &new->table);
                tableAddAll(&a->table, &new->table);
                PUSH(OBJ_VAL(new));
            } else if (IS_ARRAY(PEEK()) && IS_ARRAY(PEEK2())) {
                ObjArray* b   = AS_ARRAY(POP());
                ObjArray* a   = AS_ARRAY(POP());
                ObjArray* new = newArray();
                joinValueArray(&new->array, &a->array);
                joinValueArray(&new->array, &b->array);
                PUSH(OBJ_VAL(new));
            } else if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.addString);
            } else {
                STORE_FRAME();
                runtimeError(
                    "Operands must be two joinable types.");
                return INTERPRET_RUNTIME_ERROR;
            }
            DISPATCH();
        }

        CASE_CODE(SUBTRACT)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.subString);
            } else {
                BINARY_OP(NUMBER_VAL, -);
            }
            DISPATCH();
        }

        CASE_CODE(MULTIPLY)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.mulString);
            } else {
                BINARY_OP(NUMBER_VAL, *);
            }
            DISPATCH();
        }

        CASE_CODE(DIVIDE)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.divString);
            } else {
                BINARY_OP(NUMBER_VAL, /);
            }
            DISPATCH();
        }

        CASE_CODE(MODULO)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.modString);
            } else {
                BINARY_OP_INT(NUMBER_VAL, %);
            }
            DISPATCH();
        }

        CASE_CODE(BITWISE_AND)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.andString);
            } else {
                BINARY_OP_INT(NUMBER_VAL, &);
            }
            DISPATCH();
        }

        CASE_CODE(BITWISE_OR)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.orString);
            } else {
                BINARY_OP_INT(NUMBER_VAL, |);
            }
            DISPATCH();
        }

        CASE_CODE(BITWISE_XOR)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.xorString);
            } else {
                BINARY_OP_INT(NUMBER_VAL, ^);
            }
            DISPATCH();
        }

        CASE_CODE(SHIFT_LEFT)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.lshiftString);
            } else {
                BINARY_OP_INT(NUMBER_VAL, <<);
            }
            DISPATCH();
        }

        CASE_CODE(SHIFT_RIGHT)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.rshiftString);
            } else {
                BINARY_OP_INT(NUMBER_VAL, >>);
            }
            DISPATCH();
        }

        CASE_CODE(NOT)
            :
        {
            if (IS_INSTANCE(PEEK()) && IS_INSTANCE(PEEK2())) {
                INVOKE_DUNDER(vm.notString);
            } else {
                push(BOOL_VAL(isFalsey(pop())));
            }
            DISPATCH();
        }

        CASE_CODE(NEGATE)
            :
        {
            if (!IS_NUMBER(PEEK())) {
                STORE_FRAME();
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            DISPATCH();
        }

        CASE_CODE(INCREMENT)
            :
        {
            if (!IS_NUMBER(PEEK())) {
                STORE_FRAME();
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
            DISPATCH();
        }

        CASE_CODE(DECREMENT)
            :
        {
            if (!IS_NUMBER(PEEK())) {
                STORE_FRAME();
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(AS_NUMBER(pop()) - 1));
            DISPATCH();
        }

        CASE_CODE(POP)
            :
        {
            DROP();
            DISPATCH();
        }

        CASE_CODE(POP_N)
            :
        {
            uint8_t n = READ_BYTE();
            vm.stackTop -= n;
            DISPATCH();
        }

        CASE_CODE(DUP)
            :
        {
            PUSH(PEEK());
            DISPATCH();
        }

        CASE_CODE(GET_LOCAL)
            :
        {
            uint16_t slot = READ_SHORT();
            PUSH(stackStart[slot]);
            DISPATCH();
        }

        CASE_CODE(GET_LOCAL_2)
            :
        {
            uint16_t slotA = READ_SHORT();
            uint16_t slotB = READ_SHORT();
            PUSH(stackStart[slotA]);
            PUSH(stackStart[slotB]);
            DISPATCH();
        }

        CASE_CODE(GET_LOCAL_3)
            :
        {
            uint16_t slotA = READ_SHORT();
            uint16_t slotB = READ_SHORT();
            uint16_t slotC = READ_SHORT();
            PUSH(stackStart[slotA]);
            PUSH(stackStart[slotB]);
            PUSH(stackStart[slotC]);
            DISPATCH();
        }

        CASE_CODE(GET_LOCAL_4)
            :
        {
            uint16_t slotA = READ_SHORT();
            uint16_t slotB = READ_SHORT();
            uint16_t slotC = READ_SHORT();
            uint16_t slotD = READ_SHORT();
            PUSH(stackStart[slotA]);
            PUSH(stackStart[slotB]);
            PUSH(stackStart[slotC]);
            PUSH(stackStart[slotD]);
            DISPATCH();
        }

        CASE_CODE(SET_LOCAL)
            :
        {
            uint16_t slot    = READ_SHORT();
            stackStart[slot] = PEEK();
            DISPATCH();
        }

        CASE_CODE(SET_LOCAL_2)
            :
        {
            uint16_t slotA    = READ_SHORT();
            uint16_t slotB    = READ_SHORT();
            stackStart[slotA] = PEEK();
            stackStart[slotB] = PEEK2();
            DISPATCH();
        }

        CASE_CODE(SET_LOCAL_3)
            :
        {
            uint16_t slotA    = READ_SHORT();
            uint16_t slotB    = READ_SHORT();
            uint16_t slotC    = READ_SHORT();
            stackStart[slotA] = PEEK();
            stackStart[slotB] = PEEK2();
            stackStart[slotC] = PEEK3();
            DISPATCH();
        }

        CASE_CODE(SET_LOCAL_4)
            :
        {
            uint16_t slotA    = READ_SHORT();
            uint16_t slotB    = READ_SHORT();
            uint16_t slotC    = READ_SHORT();
            uint16_t slotD    = READ_SHORT();
            stackStart[slotA] = PEEK();
            stackStart[slotB] = PEEK2();
            stackStart[slotC] = PEEK3();
            stackStart[slotD] = PEEK4();
            DISPATCH();
        }

        CASE_CODE(GET_GLOBAL)
            :
        {
            Value name = READ_CONSTANT();
            Value value;
            if (!tableGet(&vm.globals, name, &value)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(name));
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(value);
            DISPATCH();
        }

        CASE_CODE(GET_GLOBAL_2)
            :
        {
            Value nameA = READ_CONSTANT();
            Value valueA;
            if (!tableGet(&vm.globals, nameA, &valueA)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameA));
                return INTERPRET_RUNTIME_ERROR;
            }

            Value nameB = READ_CONSTANT();
            Value valueB;
            if (!tableGet(&vm.globals, nameB, &valueB)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameB));
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(valueA);
            PUSH(valueB);
            DISPATCH();
        }

        CASE_CODE(GET_GLOBAL_3)
            :
        {
            Value nameA = READ_CONSTANT();
            Value valueA;
            if (!tableGet(&vm.globals, nameA, &valueA)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameA));
                return INTERPRET_RUNTIME_ERROR;
            }

            Value nameB = READ_CONSTANT();
            Value valueB;
            if (!tableGet(&vm.globals, nameB, &valueB)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameB));
                return INTERPRET_RUNTIME_ERROR;
            }

            Value nameC = READ_CONSTANT();
            Value valueC;
            if (!tableGet(&vm.globals, nameC, &valueC)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameC));
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(valueA);
            PUSH(valueB);
            PUSH(valueC);
            DISPATCH();
        }

        CASE_CODE(GET_GLOBAL_4)
            :
        {
            Value nameA = READ_CONSTANT();
            Value valueA;
            if (!tableGet(&vm.globals, nameA, &valueA)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameA));
                return INTERPRET_RUNTIME_ERROR;
            }

            Value nameB = READ_CONSTANT();
            Value valueB;
            if (!tableGet(&vm.globals, nameB, &valueB)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameB));
                return INTERPRET_RUNTIME_ERROR;
            }

            Value nameC = READ_CONSTANT();
            Value valueC;
            if (!tableGet(&vm.globals, nameC, &valueC)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameC));
                return INTERPRET_RUNTIME_ERROR;
            }

            Value nameD = READ_CONSTANT();
            Value valueD;
            if (!tableGet(&vm.globals, nameD, &valueD)) {
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(nameD));
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(valueA);
            PUSH(valueB);
            PUSH(valueC);
            PUSH(valueD);
            DISPATCH();
        }

        CASE_CODE(DEFINE_GLOBAL)
            :
        {
            Value name = READ_CONSTANT();
            tableSet(&vm.globals, name, PEEK());
            DROP();
            DISPATCH();
        }

        CASE_CODE(SET_GLOBAL)
            :
        {
            Value name = READ_CONSTANT();
            if (tableSet(&vm.globals, name, PEEK())) {
                tableDelete(&vm.globals, name);
                STORE_FRAME();
                runtimeError("Undefined variable '%s'.", stringValue(name));
                return INTERPRET_RUNTIME_ERROR;
            }
            DISPATCH();
        }

        CASE_CODE(GET_PROPERTY)
            :
        {
            Obj* value = AS_OBJ(PEEK()); // break
            switch (value->type) {
            case OBJ_INSTANCE: {
                ObjInstance* instance = AS_INSTANCE(PEEK());
                Value        name     = READ_CONSTANT();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    DROP();
                    PUSH(value);
                    break;
                }

                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OBJ_TABLE: {
                ObjTable* table = AS_TABLE(PEEK());
                Value     index = READ_CONSTANT();
                Value     value;
                if (!tableGet(&table->table, index, &value)) {
                    STORE_FRAME();
                    runtimeError("3Undefined property '%s'.", AS_STRING(index)->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                DROP();
                PUSH(value);
                break;
            }

            default: {
                STORE_FRAME();
                runtimeError("Only instances and tables have properties.");
                return INTERPRET_RUNTIME_ERROR;
            }
            }
            DISPATCH();
        }

        CASE_CODE(SET_PROPERTY)
            :
        {
            Obj* value = AS_OBJ(PEEK2());

            switch (value->type) {
            case OBJ_INSTANCE: {
                ObjInstance* instance = AS_INSTANCE(PEEK2());
                tableSet(&instance->fields, READ_CONSTANT(), PEEK());
                Value value = POP();
                DROP();
                PUSH(value);
                break;
            }

            case OBJ_TABLE: {
                ObjTable* table = AS_TABLE(PEEK2());
                tableSet(&table->table, READ_CONSTANT(), PEEK());
                Value value = POP();
                DROP();
                PUSH(value);
                break;
            }

            case OBJ_CLASS: {
                if (IS_CLASS(PEEK())) {
                    ObjClass* klass = AS_CLASS(PEEK());
                    printf("name: %s\n", klass->name->chars);
                    tableSet(&klass->fields, READ_CONSTANT(), PEEK2());
                    POP();
                } else {
                    ObjClass* klass = AS_CLASS(PEEK2());
                    tableSet(&klass->fields, READ_CONSTANT(), PEEK());
                    POP();
                }
                break;
            }

            default:
                STORE_FRAME();
                runtimeError("Only instances and tables have fields.");
                return INTERPRET_RUNTIME_ERROR;
            }

            DISPATCH();
        }

        CASE_CODE(GET_SUPER)
            :
        {
            Value     name       = READ_CONSTANT();
            ObjClass* superclass = AS_CLASS(POP());

            if (!bindMethod(superclass, name)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            DISPATCH();
        }

        CASE_CODE(JUMP)
            :
        {
            uint16_t offset = READ_SHORT();
            ip += offset;
            DISPATCH();
        }

        CASE_CODE(JUMP_IF_FALSE)
            :
        {
            uint16_t offset = READ_SHORT();
            if (isFalsey(PEEK()))
                ip += offset;
            DISPATCH();
        }

        CASE_CODE(LOOP)
            :
        {
            uint16_t offset = READ_SHORT();
            ip -= offset;
            DISPATCH();
        }

        CASE_CODE(DUMP)
            :
        {
            dumpValue(POP());
            printf("\n");
            DISPATCH();
        }

        CASE_CODE(CALL)
            :
        {
            int argCount = READ_SHORT();
            STORE_FRAME();

            if (!callValue(peek(argCount), argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }

            LOAD_FRAME();
            DISPATCH();
        }

        CASE_CODE(CALL_BLIND)
            :
        {
            int argCount = READ_SHORT();
            STORE_FRAME();

            if (!callValue(peek(argCount), argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }

            LOAD_FRAME();
            POP(); // CALL_BLIND is just CALL with a POP after
            DISPATCH();
        }

        CASE_CODE(INDEX)
            :
        {
            Value index = POP();
            Value value = POP();

            if (IS_OBJ(value)) {
                switch (OBJ_TYPE(value)) {
                case OBJ_STRING: {
                    if (IS_NUMBER(index)) {
                        int        i      = (int)AS_NUMBER(index);
                        ObjString* string = AS_STRING(value);
                        if (i < 0 || i >= string->length) {
                            STORE_FRAME();
                            runtimeError("String index out of bounds.");
                            return INTERPRET_RUNTIME_ERROR;
                        }

                        int          index = 0;
                        utf8_int32_t codepoint;
                        utf8_int8_t* str = utf8codepoint(string->chars, &codepoint);
                        while (str != NULL) {
                            if (index == i)
                                break;

                            str = utf8codepoint(str, &codepoint);
                            index++;
                        }

                        char* output = malloc(5);
                        utf8catcodepoint(output, codepoint, 2);

                        PUSH(OBJ_VAL(copyString(output, strlen(output))));
                    }
                    break;
                }
                case OBJ_TABLE: {
                    ObjTable* table = AS_TABLE(value);
                    Value     entry;
                    if (tableGet(&table->table, index, &entry)) {
                        PUSH(entry);
                    } else {
                        STORE_FRAME();
                        runtimeError("Undefined table property '%s'.", stringValue(index));
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    break;
                }
                case OBJ_ARRAY: {
                    ObjArray* array = AS_ARRAY(value);
                    if (IS_NUMBER(index)) {
                        unsigned int i = (unsigned int)AS_NUMBER(index);
                        if (i < 0)
                            i = array->array.count + i;
                        if (i < 0 || i >= array->array.count) {
                            STORE_FRAME();
                            runtimeError("Array index out of bounds.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        Value entry = array->array.values[i];
                        PUSH(entry);
                    }
                    break;
                }
                default:
                    STORE_FRAME();
                    runtimeError("Only strings, tables and arrays can be indexed.");
                    return INTERPRET_COMPILE_ERROR;
                }
            }

            DISPATCH();
        }

        CASE_CODE(SET_INDEX)
            :
        {
            Obj* value = AS_OBJ(peek(2));

            switch (value->type) {
            case OBJ_TABLE: {
                Value     value = POP();
                Value     index = POP();
                ObjTable* table = AS_TABLE(POP());
                tableSet(&table->table, index, value);
                PUSH(OBJ_VAL(table));
                break;
            }

            case OBJ_ARRAY: {
                Value     value = POP();
                Value     index = POP();
                ObjArray* array = AS_ARRAY(POP());
                if (!IS_NUMBER(index)) {
                    STORE_FRAME();
                    runtimeError("Index must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_NUMBER(index) < 0 || AS_NUMBER(index) >= array->array.count) {
                    STORE_FRAME();
                    runtimeError("Index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                array->array.values[(int)AS_NUMBER(index)] = value;
                PUSH(OBJ_VAL(array));
                break;
            }

            case OBJ_STRING: {
                Value      value  = POP();
                Value      index  = POP();
                ObjString* string = AS_STRING(POP());
                if (!IS_NUMBER(index)) {
                    STORE_FRAME();
                    runtimeError("Index must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_NUMBER(index) < 0 || AS_NUMBER(index) >= string->length) {
                    STORE_FRAME();
                    runtimeError("Index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!IS_STRING(value)) {
                    STORE_FRAME();
                    runtimeError("Value must be a character.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_STRING(value)->length != 1) {
                    STORE_FRAME();
                    runtimeError("Value must be a character.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                string->chars[(int)AS_NUMBER(index)] = AS_STRING(value)->chars[0];
                PUSH(OBJ_VAL(string));
                break;
            }

            default: {
                STORE_FRAME();
                runtimeError("Only strings, tables and arrays have indexes.");
                return INTERPRET_RUNTIME_ERROR;
            }
            }

            DISPATCH();
        }

        CASE_CODE(INVOKE)
            :
        {
            Value method   = READ_CONSTANT();
            int   argCount = READ_SHORT();
            STORE_FRAME();

            if (!invoke(method, argCount)) {
                vm.errorState = true;
                return INTERPRET_RUNTIME_ERROR;
            }

            LOAD_FRAME();
            DISPATCH();
        }

        CASE_CODE(SUPER_INVOKE)
            :
        {
            Value     method     = READ_CONSTANT();
            int       argCount   = READ_SHORT();
            ObjClass* superclass = AS_CLASS(POP());
            STORE_FRAME();

            if (!invokeFromClass(superclass, method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }

            LOAD_FRAME();
            DISPATCH();
        }

        CASE_CODE(CLOSURE)
            :
        {
            ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure*  closure  = newClosure(function);

            PUSH(OBJ_VAL(closure));

            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t  isLocal = READ_BYTE();
                uint16_t index   = READ_SHORT();
                if (isLocal) {
                    closure->upvalues[i] = captureUpvalue(stackStart + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            DISPATCH();
        }

        CASE_CODE(CLOSE_UPVALUE)
            :
        {
            closeUpvalues(vm.stackTop - 1);
            DROP();
            DISPATCH();
        }

        CASE_CODE(SET_TABLE)
            :
        {
            int       elemsCount = READ_SHORT();
            ObjTable* table      = newTable();

            if (elemsCount > 0) {
                for (int i = elemsCount - 1; i >= 0; i--) {
                    // if (!IS_STRING(PEEK2())) {
                    //     runtimeError("Table key must be a string.");
                    //     return INTERPRET_RUNTIME_ERROR;
                    // }
                    tableSet(&table->table, PEEK2(), PEEK());
                    DROP();
                    DROP();
                }
            }

            PUSH(OBJ_VAL(table));
            DISPATCH();
        }

        CASE_CODE(SET_ARRAY)
            :
        {
            int       elemsCount = READ_SHORT();
            ObjArray* array      = newArray();

            if (elemsCount > 0) {
                for (int i = elemsCount - 1; i >= 0; i--) {
                    writeValueArray(&array->array, peek(i));
                }

                for (int i = elemsCount - 1; i >= 0; i--) {
                    DROP();
                }
            }

            PUSH(OBJ_VAL(array));
            DISPATCH();
        }

        CASE_CODE(REENTER)
            :
        {
            Value result = POP();
            closeUpvalues(stackStart);
            vm.frameCount--;
            PUSH(result);
            return INTERPRET_OK;
        }

        CASE_CODE(RETURN)
            :
        {
            Value result = POP();
            closeUpvalues(stackStart);
            vm.frameCount--;
            if (vm.frameCount == 0) {
                DROP();
                return INTERPRET_OK;
            }

            vm.stackTop = stackStart;
            PUSH(result);

            LOAD_FRAME();
            DISPATCH();
        }

        CASE_CODE(CLASS)
            :
        {
            PUSH(OBJ_VAL(newClass(READ_STRING())));
            DISPATCH();
        }

        CASE_CODE(INHERIT)
            :
        {
            if (!IS_CLASS(PEEK2())) {
                STORE_FRAME();
                runtimeError("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjClass* subClass   = AS_CLASS(PEEK());
            ObjClass* superClass = AS_CLASS(PEEK2());

            tableAddAll(&superClass->methods, &subClass->methods);
            tableAddAll(&superClass->fields, &subClass->fields);
            POP(); // Subclass.
            DISPATCH();
        }

        CASE_CODE(METHOD)
            :
        {
            defineMethod(READ_STRING());
            DISPATCH();
        }

        CASE_CODE(SLICE)
            :
        {
            Value end   = POP();
            Value start = POP();
            if (!IS_NUMBER(start) || !IS_NUMBER(end)) {
                STORE_FRAME();
                runtimeError("Slice bounds must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }

            if (!valueSlice(start, end)) {
                return INTERPRET_RUNTIME_ERROR;
            }

            DISPATCH();
        }

        CASE_CODE(FORMAT)
            :
        {
            uint16_t argCount   = READ_SHORT();
            ObjString* template = AS_STRING(peek(argCount));

            char* buffer = (char*)malloc(template->length + TEMPLATE_BUFFER * sizeof(char));
            if (buffer == NULL) {
                fprintf(stderr, "Malloc failed!\n");
                return INTERPRET_RUNTIME_ERROR;
            }

            strcpy(buffer, template->chars);

            for (int i = argCount; i >= 0; i--) {
                Value arg    = peek(i - 1);
                char* string = stringValue(arg);
                replace_placeholder(buffer, string);
            }

            for (int i = 0; i < argCount + 1; i++)
                POP();

            PUSH(OBJ_VAL(takeString(buffer, strlen(buffer))));

            DISPATCH();
        }

        CASE_CODE(IMPORT)
            :
        {
            ObjString*   fileName   = AS_STRING(POP());
            ObjFunction* parentFunc = fn;
            const char*  sourcePath = resolveRelativePath(fileName->chars, parentFunc->source);
            char*        source     = readFile(sourcePath);
            ObjFunction* function   = compile(sourcePath, source);
            if (function == NULL)
                return INTERPRET_COMPILE_ERROR;
            PUSH(OBJ_VAL(function));
            ObjClosure* closure = newClosure(function);
            POP();
            STORE_FRAME();
            call(closure, 0);
            LOAD_FRAME();
            free(source);
            DISPATCH();
        }
    }
#undef INTERPRET_LOOP
#undef CASE_CODE
#undef DISPATCH
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef BINARY_OP_INT
#undef INVOKE_DUNDER
#undef PUSH
#undef POP
#undef PEEK
#undef PEEK2
#undef PEEK3
#undef PEEK4
#undef DROP
#undef STORE_FRAME
#undef LOAD_FRAME
}

InterpretResult interpret(const char* sourcePath, utf8_int8_t* source)
{
    ObjFunction* function = compile(sourcePath, source);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));

    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    if (call(closure, 0) && !vm.errorState) {
        return run();
    }

    return INTERPRET_RUNTIME_ERROR;
}
