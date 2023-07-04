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

void writeValueArrayAt(ValueArray* array, Value value, int index)
{
    if (index >= array->count || index < 0) {
        return;
    }
    array->values[index] = value;
}

Value removeValueArrayAt(ValueArray* array, int index)
{
    Value value = array->values[index];
    for (int i = index; i < array->count - 1; i++) {
        array->values[i] = array->values[i + 1];
    }

    array->count--;
    return value;
}

void joinValueArray(ValueArray* array, ValueArray* other)
{
    for (int i = 0; i < other->count; i++) {
        writeValueArray(array, other->values[i]);
    }
}

void copyValueArray(ValueArray* old, ValueArray* new, int start, int end)
{
    for (int i = start; i <= end; i++) {
        writeValueArray(new, old->values[i]);
    }
}

void freeValueArray(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

bool arraysEqual(ValueArray* a, ValueArray* b)
{
    if (a->count != b->count) {
        return false;
    }

    for (int i = 0; i < a->count; i++) {
        if (IS_ARRAY(a->values[i]) && IS_ARRAY(b->values[i])) {
            if (!arraysEqual(&AS_ARRAY(a->values[i])->array, &AS_ARRAY(b->values[i])->array)) {
                return false;
            }
        } else if (!valuesEqual(a->values[i], b->values[i])) {
            return false;
        }
    }

    return true;
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
    case VAL_POINTER:
        return AS_POINTER(value);
    }
#endif

    return 0;
}

void printValueArray(ValueArray* array)
{
    printf("[ ");
    for (int i = 0; i < array->count; i++) {
        dumpValue(array->values[i]);
        if (i < array->count - 1) {
            printf(", ");
        }
        printf(" ");
    }
    printf("]");
}

const char* valueType(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        return "bool";
    } else if (IS_NIL(value)) {
        return "nil";
    } else if (IS_NUMBER(value)) {
        return "number";
    } else if (IS_OBJ(value)) {
        return objectType(value);
    } else if (IS_EMPTY(value)) {
        return "empty";
    } else if (IS_POINTER(value)) {
        return "pointer";
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        return "bool";
    case VAL_NIL:
        return "nil";
    case VAL_NUMBER:
        return "number";
    case VAL_OBJ:
        return "object";
    case VAL_EMPTY:
        return "empty";
    case VAL_POINTER:
        return "pointer";
    default:
        return "unknown";
    }
#endif

    return "unknown";
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
    } else if (IS_POINTER(value)) {
        printf("*%p", AS_POINTER(value));
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
    case VAL_POINTER:
        printf("*%p", (void*)AS_POINTER(value));
        break;
    }
#endif
}

void dumpValue(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        printf("nil");
    } else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        dumpObject(value);
    } else if (IS_EMPTY(value)) {
        printf("empty");
    } else if (IS_POINTER(value)) {
        printf("*%p", AS_POINTER(value));
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
    case VAL_POINTER:
        printf("*%p", (void*)AS_POINTER(value));
        break;
    }
#endif
}

char* format_double(double val)
{
    char* buffer = malloc(sizeof(char) * 64);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    sprintf(buffer, "%.10f", val);

    // Trimming from the end
    char* ptr = &buffer[strlen(buffer) - 1];
    while (*ptr == '0') {
        *ptr = '\0';
        ptr--;
    }

    // If the last character is '.', also remove it
    if (*ptr == '.') {
        *ptr = '\0';
    }

    return buffer;
}

char* stringValue(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        return AS_BOOL(value) ? "true" : "false";
    } else if (IS_NIL(value)) {
        return "nil";
    } else if (IS_NUMBER(value)) {
        return format_double(AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        return objectString(value);
    } else if (IS_EMPTY(value)) {
        return "empty";
    } else if (IS_POINTER(value)) {
        char* str = malloc(sizeof(char) * 50);
        sprintf(str, "*%p", AS_POINTER(value));
        return str;
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
    case VAL_POINTER: {
        char* str = malloc(sizeof(char) * 50);
        sprintf(str, "*%p", (void*)AS_POINTER(value));
        return str;
    }
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
