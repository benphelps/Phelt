#ifndef LUX_NATIVE_H
#define LUX_NATIVE_H

#include "common.h"
#include "object.h"
#include "value.h"

typedef struct {
    const char* name;
    NativeFn    function;
} NativeFnEntry;

extern NativeFnEntry nativeFns[];

#endif
