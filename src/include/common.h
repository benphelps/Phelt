#ifndef phelt_common_h
#define phelt_common_h

// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

#define NAN_BOXING
#define UINT8_COUNT (UINT8_MAX + 1)
#define COMPUTED_GOTO
#define HASH_FUNCTION HASH_MURMUR3 // or HASH_FNV1A

#define TEMPLATE_BUFFER 1024

#include "utf8.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

utf8_int8_t* readFile(const char* path);
bool         fileExists(const char* path);
const char*  resolvePath(const char* path);
const char*  getFilePath(const char* path);
const char*  resolveRelativePath(const char* path, const char* currentFile);

#define UNUSED(x) (void)(x)

#endif
