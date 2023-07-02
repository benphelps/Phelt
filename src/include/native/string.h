#ifndef PHELT_NATIVE_STRING_H
#define PHELT_NATIVE_STRING_H

#include "native.h"
#include "ph_string.h"
#include "utf8.h"

extern bool string_length(int argCount, Value* args);
extern bool string_sub(int argCount, Value* args);
extern bool string_find(int argCount, Value* args);
extern bool string_replace(int argCount, Value* args);
extern bool string_split(int argCount, Value* args);
extern bool string_trim(int argCount, Value* args);
extern bool string_upper(int argCount, Value* args);
extern bool string_lower(int argCount, Value* args);
extern bool string_reverse(int argCount, Value* args);
extern bool string_repeat(int argCount, Value* args);

#endif
