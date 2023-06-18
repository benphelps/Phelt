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

extern bool _time(int argCount, Value* args);
extern bool _clock(int argCount, Value* args);
extern bool _sleep(int argCount, Value* args);
extern bool _usleep(int argCount, Value* args);
extern bool _print(int argCount, Value* args);
extern bool _println(int argCount, Value* args);
extern bool _sprint(int argCount, Value* args);

extern bool _ceil(int argCount, Value* args);
extern bool _floor(int argCount, Value* args);
extern bool _abs(int argCount, Value* args);
extern bool _exp(int argCount, Value* args);
extern bool _sqrt(int argCount, Value* args);
extern bool _sin(int argCount, Value* args);
extern bool _cos(int argCount, Value* args);
extern bool _tan(int argCount, Value* args);
extern bool _atan(int argCount, Value* args);
extern bool _pow(int argCount, Value* args);
extern bool _atan2(int argCount, Value* args);

#endif
