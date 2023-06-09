#ifndef lux_compiler_h
#define lux_compiler_h

#include "chunk.h"
#include "object.h"

#define MAX_CASES 256

ObjFunction* compile(const char* source);

#endif
