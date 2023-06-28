#include "object.h"
#include "common.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include <libgen.h>

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type)
{
    Obj* object      = (Obj*)reallocate(NULL, 0, size);
    object->isMarked = false;
    object->type     = type;
    object->next     = vm.objects;
    vm.objects       = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method)
{
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver       = receiver;
    bound->method         = method;
    return bound;
}

ObjClass* newClass(ObjString* name)
{
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name     = name;
    initTable(&klass->methods);
    initTable(&klass->fields);
    return klass;
}

ObjInstance* newInstance(ObjClass* klass)
{
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass       = klass;
    initTable(&instance->fields);
    tableAddAll(&instance->klass->fields, &instance->fields);
    return instance;
}

ObjClosure* newClosure(ObjFunction* function)
{
    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);

    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*,
        function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    closure->function     = function;
    closure->upvalues     = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction(void)
{
    ObjFunction* function  = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity        = 0;
    function->upvalueCount = 0;
    function->name         = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjNative* newNative(NativeFn function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function  = function;
    return native;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash)
{
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length    = length;
    string->chars     = chars;
    string->hash      = hash;
    push(OBJ_VAL(string));
    tableSet(&vm.strings, OBJ_VAL(string), NIL_VAL);
    pop();
    return string;
}

static uint32_t hashString(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* takeString(char* chars, int length)
{
    uint32_t   hash     = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length)
{
    uint32_t   hash     = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
        return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjString* formatString(const char* format, ...)
{
    char*   buffer = NULL;
    va_list args;
    va_start(args, format);

    int len = vsnprintf(NULL, 0, format, args);
    if (len < 0) {
        return NULL;
    }

    buffer = malloc(len + 1);
    if (!buffer) {
        return NULL;
    }

    va_end(args);
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);

    uint32_t   hash     = hashString(buffer, len);
    ObjString* interned = tableFindString(&vm.strings, buffer, len, hash);
    if (interned != NULL)
        return interned;

    char* heapChars = ALLOCATE(char, len + 1);
    memcpy(heapChars, buffer, len);
    heapChars[len] = '\0';
    return allocateString(heapChars, len, hash);
}

ObjUpvalue* newUpvalue(Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed     = NIL_VAL;
    upvalue->location   = slot;
    upvalue->next       = NULL;
    return upvalue;
}

ObjTable* newTable(void)
{
    ObjTable* table = ALLOCATE_OBJ(ObjTable, OBJ_TABLE);
    initTable(&table->table);
    return table;
}

ObjArray* newArray(void)
{
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    initValueArray(&array->array);
    return array;
}

static void printFunction(ObjFunction* function)
{
    if (function->name == NULL) {
        printf("<%s>", basename((char*)function->source));
        return;
    }
    printf("<fn %s>", function->name->chars);
}

char* functionString(ObjFunction* function)
{
    if (function->name == NULL) {
        return basename((char*)function->source);
    }
    char* name = ALLOCATE(char, function->name->length + 1);
    memcpy(name, function->name->chars, function->name->length);
    name[function->name->length] = '\0';
    return name;
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
        printFunction(AS_BOUND_METHOD(value)->method->function);
        break;
    case OBJ_CLASS:
        printf("%s", AS_CLASS(value)->name->chars);
        break;
    case OBJ_INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
        break;
    case OBJ_CLOSURE:
        printFunction(AS_CLOSURE(value)->function);
        break;
    case OBJ_FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
    case OBJ_NATIVE:
        printf("<native fn>");
        break;
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    case OBJ_TABLE:
        printf("table");
        break;
    case OBJ_ARRAY:
        printf("array");
        break;
    case OBJ_UPVALUE:
        printf("upvalue");
        break;
    }
}

void dumpObject(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
        printFunction(AS_BOUND_METHOD(value)->method->function);
        break;
    case OBJ_CLASS:
        printf("%s", AS_CLASS(value)->name->chars);
        break;
    case OBJ_INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
        break;
    case OBJ_CLOSURE:
        printFunction(AS_CLOSURE(value)->function);
        break;
    case OBJ_FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
    case OBJ_NATIVE:
        printf("<native fn>");
        break;
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    case OBJ_TABLE:
        printTable(&AS_TABLE(value)->table);
        break;
    case OBJ_ARRAY:
        printValueArray(&AS_ARRAY(value)->array);
        break;
    case OBJ_UPVALUE:
        printf("upvalue");
        break;
    }
}

char* objectString(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
        return functionString(AS_BOUND_METHOD(value)->method->function);
    case OBJ_CLASS:
        return AS_CLASS(value)->name->chars;
    case OBJ_INSTANCE: {

        ObjInstance* instance = AS_INSTANCE(value);
        Value        method   = OBJ_VAL(vm.strString);

        Value value;
        if (!tableGet(&instance->klass->methods, method, &value)) {
            printf("Could not find method %s on class %s\n", AS_CSTRING(method), AS_CSTRING(OBJ_VAL(instance->klass->name)));
            return instance->klass->name->chars;
        }

        ObjBoundMethod* bound = newBoundMethod(value, AS_CLOSURE(value));

        // patch the function to reenter the VM, instead of returning
        bound->method->function->chunk.code[bound->method->function->chunk.count - 1] = OP_REENTER;
        call(bound->method, 0);
        run();
        return AS_CSTRING(pop());
    }
    case OBJ_CLOSURE:
        return functionString(AS_CLOSURE(value)->function);
    case OBJ_FUNCTION:
        return functionString(AS_FUNCTION(value));
    case OBJ_NATIVE:
        return "<native fn>";
    case OBJ_STRING:
        return AS_CSTRING(value);
    case OBJ_TABLE:
        return "table";
    case OBJ_ARRAY:
        return "array";
    case OBJ_UPVALUE:
        return "upvalue";
    }
}

int objectLength(Value object)
{
    switch (AS_OBJ(object)->type) {
    case OBJ_BOUND_METHOD:
    case OBJ_CLASS:
    case OBJ_INSTANCE:
    case OBJ_CLOSURE:
    case OBJ_FUNCTION:
    case OBJ_NATIVE:
    case OBJ_UPVALUE:
        return -1;
    case OBJ_STRING:
        return utf8len(AS_CSTRING(object));
    case OBJ_TABLE:
        return AS_TABLE(object)->table.count;
    case OBJ_ARRAY:
        return AS_ARRAY(object)->array.count;
    }
}
