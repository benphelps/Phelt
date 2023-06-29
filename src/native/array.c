#include "native/array.h"

bool array_length(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);
    phelt_pushNumber(-1, array->array.count);
    return true;
}

bool array_push(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);
    writeValueArray(&array->array, phelt_value(1));
    return true;
}

bool array_pop(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);
    if (array->array.count == 0) {
        phelt_pushObject(-1, "Array is empty.");
        return false;
    }

    phelt_pushValue(-1, array->array.values[array->array.count - 1]);
    array->array.count--;
    return true;
}

bool array_insert(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkArray(0);
    phelt_checkNumber(1);

    ObjArray* array = phelt_toArray(0);
    int       index = (int)phelt_toNumber(1);
    if (index < 0 || index > array->array.count) {
        phelt_error("Index out of bounds.");
        return false;
    }

    writeValueArrayAt(&array->array, phelt_value(2), index);
    return true;
}

bool array_remove(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);
    phelt_checkNumber(1);

    ObjArray* array = phelt_toArray(0);
    int       index = (int)phelt_toNumber(1);
    if (index < 0 || index >= array->array.count) {
        phelt_error("Index out of bounds.");
        return false;
    }

    Value removed = removeValueArrayAt(&array->array, index);
    phelt_pushValue(-1, removed);
    return true;
}

bool array_sort(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);
    phelt_checkClosure(1);

    ObjArray*   array = phelt_toArray(0);
    ObjClosure* func  = phelt_toClosure(1);

    if (array->array.count == 0) {
        return true;
    }

    for (int i = 0; i < array->array.count - 1; i++) {
        for (int j = i + 1; j < array->array.count; j++) {
            push(array->array.values[i]);
            push(array->array.values[j]);
            phelt_callClosure(func, 2);
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

bool array_reverse(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);

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

bool array_find(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);

    if (array->array.count == 0) {
        phelt_pushBool(-1, false);
        return true;
    }

    for (int i = 0; i < array->array.count; i++) {
        if (valuesEqual(array->array.values[i], phelt_value(1))) {
            phelt_pushNumber(-1, i);
            return true;
        }
    }

    phelt_pushBool(-1, false);
    return true;
}

bool array_findLast(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);

    if (array->array.count == 0) {
        phelt_pushBool(-1, false);
        return true;
    }

    for (int i = array->array.count - 1; i >= 0; i--) {
        if (valuesEqual(array->array.values[i], phelt_value(1))) {
            phelt_pushNumber(-1, i);
            return true;
        }
    }

    phelt_pushBool(-1, false);
    return true;
}

bool array_map(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);
    phelt_checkClosure(1);

    ObjArray*   array = phelt_toArray(0);
    ObjClosure* func  = phelt_toClosure(1);

    ObjArray* mapped       = newArray();
    mapped->array.capacity = array->array.capacity;
    mapped->array.count    = array->array.count;
    mapped->array.values   = ALLOCATE(Value, array->array.capacity);

    for (int i = 0; i < array->array.count; i++) {
        push(array->array.values[i]);
        phelt_callClosure(func, 1);
        pop();                           // pop value
        mapped->array.values[i] = pop(); // pop result
    }

    phelt_pushObject(-1, mapped);
    return true;
}

bool array_filter(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkArray(0);
    phelt_checkClosure(1);

    ObjArray*   array = phelt_toArray(0);
    ObjClosure* func  = phelt_toClosure(1);

    ObjArray* filtered       = newArray();
    filtered->array.capacity = array->array.capacity;
    filtered->array.count    = 0;
    filtered->array.values   = ALLOCATE(Value, array->array.capacity);

    for (int i = 0; i <= array->array.count; i++) {
        push(array->array.values[i]);
        phelt_callClosure(func, 1);
        pop();                        // pop value
        bool result = AS_BOOL(pop()); // pop result
        if (result) {
            writeValueArray(&filtered->array, array->array.values[i]);
        }
    }

    phelt_pushObject(-1, filtered);
    return true;
}

bool array_reduce(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkArray(0);
    phelt_checkClosure(1);
    phelt_checkNumber(2);

    ObjArray*   array = phelt_toArray(0);
    ObjClosure* func  = phelt_toClosure(1);
    Value       acc   = phelt_value(2);

    for (int i = 0; i <= array->array.count; i++) {
        push(acc);                    // push acc
        push(array->array.values[i]); // push value
        phelt_callClosure(func, 2);
        pop();       // pop value
        pop();       // pop acc
        acc = pop(); // pop result, set acc
    }

    phelt_pushValue(-1, acc);
    return true;
}

// flatten an array of arrays, with optional depth
bool array_flatten(int argCount, Value* args)
{
    phelt_checkMinArgs(1);
    phelt_checkArray(0);

    ObjArray* array = phelt_toArray(0);
    int       depth = -1;

    if (argCount == 2) {
        phelt_checkNumber(1);
        depth = (int)phelt_toNumber(1);
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

    phelt_pushObject(-1, flattened);
    return true;
}
