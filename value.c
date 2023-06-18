#include "value.h"
#include "memory.h"
#include "object.h"

void initValueArray(ValueArray* array)
{
    array->values   = NULL;
    array->capacity = 0;
    array->count    = 0;
}

void writeValueArray(ValueArray* array, Value value)
{
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values   = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void joinValueArray(ValueArray* array, ValueArray* other)
{
    for (int i = 0; i < other->count; i++) {
        writeValueArray(array, other->values[i]);
    }
}

void freeValueArray(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

static uint32_t hashDouble(double value)
{
    union BitCast {
        double   value;
        uint32_t ints[2];
    };

    union BitCast cast;
    cast.value = (value) + 1.0;
    return cast.ints[0] + cast.ints[1];
}

uint32_t hashValue(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        return AS_BOOL(value) ? 3 : 5;
    } else if (IS_NIL(value)) {
        return 7;
    } else if (IS_NUMBER(value)) {
        return hashDouble(AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        return AS_STRING(value)->hash;
    } else if (IS_EMPTY(value)) {
        return 0;
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        return AS_BOOL(value) ? 3 : 5;
    case VAL_NIL:
        return 7;
    case VAL_NUMBER:
        return hashDouble(AS_NUMBER(value));
    case VAL_OBJ:
        return AS_STRING(value)->hash;
    case VAL_EMPTY:
        return 0;
    }
#endif

    return 0;
}

void printValueArray(ValueArray* array)
{
    printf("[ ");
    for (int i = 0; i < array->count; i++) {
        printValue(array->values[i]);
        printf(" ");
    }
    printf("]");
}

void printValue(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        printf("nil");
    } else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        printObject(value);
    } else if (IS_EMPTY(value)) {
        printf("empty");
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        printObject(value);
        break;
    case VAL_EMPTY:
        printf("empty");
        break;
    }
#endif
}

char* stringValue(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        return AS_BOOL(value) ? "true" : "false";
    } else if (IS_NIL(value)) {
        return "nil";
    } else if (IS_NUMBER(value)) {
        char* str = malloc(sizeof(char) * 50);
        sprintf(str, "%g", AS_NUMBER(value));
        return str;
    } else if (IS_OBJ(value)) {
        return objectString(value);
    } else if (IS_EMPTY(value)) {
        return "empty";
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        return AS_BOOL(value) ? "true" : "false";
    case VAL_NIL:
        return "nil";
    case VAL_NUMBER: {
        char* str = malloc(sizeof(char) * 50);
        sprintf(str, "%g", AS_NUMBER(value));
        return str;
    }
    case VAL_OBJ:
        return objectString(value);
    case VAL_EMPTY:
        return "empty";
    }
#endif
    return NULL;
}

bool valuesEqual(Value a, Value b)
{
#ifdef NAN_BOXING
    return a == b;
#else
    if (a.type != b.type)
        return false;
    switch (a.type) {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
        return AS_OBJ(a) == AS_OBJ(b);
    case VAL_EMPTY:
        return true;
    default:
        return false; // Unreachable.
    }
#endif
}
