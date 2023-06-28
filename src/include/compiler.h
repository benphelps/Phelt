#ifndef phelt_compiler_h
#define phelt_compiler_h

#include "chunk.h"
#include "common.h"
#include "object.h"
#include "utf8.h"

#define MAX_CASES 256

ObjFunction* compile(const char* sourcePath, utf8_int8_t* source);
void         markCompilerRoots(void);

#endif
