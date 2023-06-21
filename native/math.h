#ifndef LUX_NATIVE_MATH_H
#define LUX_NATIVE_MATH_H

#include "native.h"

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
extern bool _deg(int argCount, Value* args);
extern bool _rad(int argCount, Value* args);
extern bool _clamp(int argCount, Value* args);
extern bool _lerp(int argCount, Value* args);
extern bool _map(int argCount, Value* args);
extern bool _norm(int argCount, Value* args);

#endif
