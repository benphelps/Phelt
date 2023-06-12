#ifndef LUX_NATIVE_H
#define LUX_NATIVE_H

#include "../common.h"
#include "../object.h"
#include "../value.h"
#include "../vm.h"

typedef struct {
    const char* name;
    NativeFn    function;
} NativeFnEntry;

extern NativeFnEntry nativeFns[];

extern Value _time(int argCount, Value* args);
extern Value _sleep(int argCount, Value* args);
extern Value _print(int argCount, Value* args);

extern Value _ceil(int argCount, Value* args);
extern Value _floor(int argCount, Value* args);
extern Value _abs(int argCount, Value* args);
extern Value _exp(int argCount, Value* args);
extern Value _sqrt(int argCount, Value* args);
extern Value _sin(int argCount, Value* args);
extern Value _cos(int argCount, Value* args);
extern Value _tan(int argCount, Value* args);
extern Value _atan(int argCount, Value* args);
extern Value _pow(int argCount, Value* args);
extern Value _atan2(int argCount, Value* args);

#endif
