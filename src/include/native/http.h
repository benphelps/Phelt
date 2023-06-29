#ifndef PHELT_NATIVE_HTTP_H
#define PHELT_NATIVE_HTTP_H

#include "native.h"

extern bool http_get(int argCount, Value* args);
extern bool http_post(int argCount, Value* args);
extern bool http_put(int argCount, Value* args);
extern bool http_patch(int argCount, Value* args);
extern bool http_delete(int argCount, Value* args);
extern bool http_head(int argCount, Value* args);
extern bool http_options(int argCount, Value* args);

#endif
