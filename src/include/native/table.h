#ifndef PHELT_NATIVE_TABLE_H
#define PHELT_NATIVE_TABLE_H

#include "native.h"

extern bool table_length(int argCount, Value* args);
extern bool table_keys(int argCount, Value* args);
extern bool table_values(int argCount, Value* args);
extern bool table_hasKey(int argCount, Value* args);
extern bool table_remove(int argCount, Value* args);
extern bool table_insert(int argCount, Value* args);

#endif
