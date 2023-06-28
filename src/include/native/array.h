#ifndef PHELT_NATIVE_ARRAY_H
#define PHELT_NATIVE_ARRAY_H

#include "native.h"

extern bool _array_push(int argCount, Value* args);
extern bool _array_pop(int argCount, Value* args);
extern bool _array_insert(int argCount, Value* args);
extern bool _array_remove(int argCount, Value* args);
extern bool _array_sort(int argCount, Value* args);
extern bool _array_reverse(int argCount, Value* args);
extern bool _array_find(int argCount, Value* args);
extern bool _array_findLast(int argCount, Value* args);
extern bool _array_map(int argCount, Value* args);
extern bool _array_filter(int argCount, Value* args);
extern bool _array_reduce(int argCount, Value* args);
extern bool _array_flatten(int argCount, Value* args);

#endif
