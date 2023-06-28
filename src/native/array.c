#include "native/array.h"

bool _array_push(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);

    ObjArray* array = lux_toArray(0);
    writeValueArray(&array->array, lux_value(1));
    return true;
}

bool _array_pop(int argCount, Value* args)
{
    lux_checkArgs(1);
    lux_checkArray(0);

    ObjArray* array = lux_toArray(0);
    if (array->array.count == 0) {
        lux_pushObject(-1, "Array is empty.");
        return false;
    }

    lux_pushValue(-1, array->array.values[array->array.count - 1]);
    array->array.count--;
    return true;
}

bool _array_insert(int argCount, Value* args)
{
    lux_checkArgs(3);
    lux_checkArray(0);
    lux_checkNumber(1);

    ObjArray* array = lux_toArray(0);
    int       index = (int)lux_toNumber(1);
    if (index < 0 || index > array->array.count) {
        lux_error("Index out of bounds.");
        return false;
    }

    writeValueArrayAt(&array->array, lux_value(2), index);
    return true;
}

bool _array_remove(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);
    lux_checkNumber(1);

    ObjArray* array = lux_toArray(0);
    int       index = (int)lux_toNumber(1);
    if (index < 0 || index >= array->array.count) {
        lux_error("Index out of bounds.");
        return false;
    }

    Value removed = removeValueArrayAt(&array->array, index);
    lux_pushValue(-1, removed);
    return true;
}

bool _array_sort(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);
    lux_checkClosure(1);

    ObjArray*   array = lux_toArray(0);
    ObjClosure* func  = lux_toClosure(1);

    if (array->array.count == 0) {
        return true;
    }

    for (int i = 0; i < array->array.count - 1; i++) {
        for (int j = i + 1; j < array->array.count; j++) {
            push(array->array.values[i]);
            push(array->array.values[j]);
            lux_callClosure(func, 2);
            pop();                        // pop values
            pop();                        // pop values
            bool result = AS_BOOL(pop()); // pop result
            if (result) {
                Value temp             = array->array.values[i];
                array->array.values[i] = array->array.values[j];
                array->array.values[j] = temp;
            }
        }
    }

    return true;
}

bool _array_reverse(int argCount, Value* args)
{
    lux_checkArgs(1);
    lux_checkArray(0);

    ObjArray* array = lux_toArray(0);

    if (array->array.count == 0) {
        return true;
    }

    for (int i = 0; i < array->array.count / 2; i++) {
        Value temp                                      = array->array.values[i];
        array->array.values[i]                          = array->array.values[array->array.count - i - 1];
        array->array.values[array->array.count - i - 1] = temp;
    }

    return true;
}

bool _array_find(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);

    ObjArray* array = lux_toArray(0);

    if (array->array.count == 0) {
        lux_pushBool(-1, false);
        return true;
    }

    for (int i = 0; i < array->array.count; i++) {
        if (valuesEqual(array->array.values[i], lux_value(1))) {
            lux_pushNumber(-1, i);
            return true;
        }
    }

    lux_pushBool(-1, false);
    return true;
}

bool _array_findLast(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);

    ObjArray* array = lux_toArray(0);

    if (array->array.count == 0) {
        lux_pushBool(-1, false);
        return true;
    }

    for (int i = array->array.count - 1; i >= 0; i--) {
        if (valuesEqual(array->array.values[i], lux_value(1))) {
            lux_pushNumber(-1, i);
            return true;
        }
    }

    lux_pushBool(-1, false);
    return true;
}

bool _array_map(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);
    lux_checkClosure(1);

    ObjArray*   array = lux_toArray(0);
    ObjClosure* func  = lux_toClosure(1);

    ObjArray* mapped       = newArray();
    mapped->array.capacity = array->array.capacity;
    mapped->array.count    = array->array.count;
    mapped->array.values   = ALLOCATE(Value, array->array.capacity);

    for (int i = 0; i < array->array.count; i++) {
        push(array->array.values[i]);
        lux_callClosure(func, 1);
        pop();                           // pop value
        mapped->array.values[i] = pop(); // pop result
    }

    lux_pushObject(-1, mapped);
    return true;
}

bool _array_filter(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkArray(0);
    lux_checkClosure(1);

    ObjArray*   array = lux_toArray(0);
    ObjClosure* func  = lux_toClosure(1);

    ObjArray* filtered       = newArray();
    filtered->array.capacity = array->array.capacity;
    filtered->array.count    = 0;
    filtered->array.values   = ALLOCATE(Value, array->array.capacity);

    for (int i = 0; i <= array->array.count; i++) {
        push(array->array.values[i]);
        lux_callClosure(func, 1);
        pop();                        // pop value
        bool result = AS_BOOL(pop()); // pop result
        if (result) {
            writeValueArray(&filtered->array, array->array.values[i]);
        }
    }

    lux_pushObject(-1, filtered);
    return true;
}

bool _array_reduce(int argCount, Value* args)
{
    lux_checkArgs(3);
    lux_checkArray(0);
    lux_checkClosure(1);
    lux_checkNumber(2);

    ObjArray*   array = lux_toArray(0);
    ObjClosure* func  = lux_toClosure(1);
    Value       acc   = lux_value(2);

    for (int i = 0; i <= array->array.count; i++) {
        push(acc);                    // push acc
        push(array->array.values[i]); // push value
        lux_callClosure(func, 2);
        pop();       // pop value
        pop();       // pop acc
        acc = pop(); // pop result, set acc
    }

    lux_pushValue(-1, acc);
    return true;
}

// flatten an array of arrays, with optional depth
bool _array_flatten(int argCount, Value* args)
{
    lux_checkMinArgs(1);
    lux_checkArray(0);

    ObjArray* array = lux_toArray(0);
    int       depth = -1;

    if (argCount == 2) {
        lux_checkNumber(1);
        depth = (int)lux_toNumber(1);
    }

    ObjArray* flattened;
    bool      hadSubArrays = false;
    int       currentDepth = 0;

    do {
        ObjArray* current       = newArray();
        current->array.capacity = array->array.capacity;
        current->array.count    = 0;
        current->array.values   = ALLOCATE(Value, array->array.capacity);
        hadSubArrays            = false;

        for (int i = 0; i < array->array.count; i++) {
            if (IS_ARRAY(array->array.values[i])) {
                ObjArray* subArray = AS_ARRAY(array->array.values[i]);
                for (int j = 0; j < subArray->array.count; j++) {
                    writeValueArray(&current->array, subArray->array.values[j]);
                }
                hadSubArrays = true;
            } else {
                writeValueArray(&current->array, array->array.values[i]);
            }
        }

        array     = current;
        flattened = array;
        currentDepth++;
    } while ((currentDepth < depth || depth == -1) && hadSubArrays);

    lux_pushObject(-1, flattened);
    return true;
}
