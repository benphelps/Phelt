#ifndef PHELT_NATIVE_ARRAY_H
#define PHELT_NATIVE_ARRAY_H

#include "native.h"

extern bool array_length(int argCount, Value* args);
extern bool array_push(int argCount, Value* args);
extern bool array_pop(int argCount, Value* args);
extern bool array_insert(int argCount, Value* args);
extern bool array_remove(int argCount, Value* args);
extern bool array_sort(int argCount, Value* args);
extern bool array_reverse(int argCount, Value* args);
extern bool array_find(int argCount, Value* args);
extern bool array_findLast(int argCount, Value* args);
extern bool array_map(int argCount, Value* args);
extern bool array_filter(int argCount, Value* args);
extern bool array_reduce(int argCount, Value* args);
extern bool array_flatten(int argCount, Value* args);

#endif
