#ifndef PHELT_NATIVE_JSON_H
#define PHELT_NATIVE_JSON_H

#include "../json.h"
#include "native.h"

extern bool json_decode(int argCount, Value* args);
extern bool json_encode(int argCount, Value* args);

#endif
