#ifndef LUX_NATIVE_HTTP_H
#define LUX_NATIVE_HTTP_H

#include "native.h"

extern bool _get(int argCount, Value* args);
extern bool _post(int argCount, Value* args);
extern bool _put(int argCount, Value* args);
extern bool _patch(int argCount, Value* args);
extern bool _delete(int argCount, Value* args);
extern bool _head(int argCount, Value* args);
extern bool _options(int argCount, Value* args);

#endif
