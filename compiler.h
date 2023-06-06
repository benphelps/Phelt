#ifndef lux_compiler_h
#define lux_compiler_h

#include "chunk.h"

#define MAX_CASES 64

bool compile(const char* source, Chunk* chunk);

#endif
