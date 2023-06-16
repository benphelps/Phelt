#include "object.h"
#include "common.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

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
    return klass;
}

ObjInstance* newInstance(ObjClass* klass)
{
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass       = klass;
    initTable(&instance->fields);
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

ObjFunction* newFunction()
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
    tableSet(&vm.strings, string, NIL_VAL);
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

ObjUpvalue* newUpvalue(Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed     = NIL_VAL;
    upvalue->location   = slot;
    upvalue->next       = NULL;
    return upvalue;
}

ObjTable* newTable()
{
    ObjTable* table = ALLOCATE_OBJ(ObjTable, OBJ_TABLE);
    initTable(&table->table);
    return table;
}

ObjArray* newArray()
{
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    initValueArray(&array->array);
    return array;
}

static void printFunction(ObjFunction* function)
{
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

char* functionString(ObjFunction* function)
{
    if (function->name == NULL) {
        return "<script>";
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
        printf("%s instance",
            AS_INSTANCE(value)->klass->name->chars);
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
    case OBJ_INSTANCE:
        return AS_INSTANCE(value)->klass->name->chars;
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
