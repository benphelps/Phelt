#ifndef PHELT_NATIVE_MATH_H
#define PHELT_NATIVE_MATH_H

#include "native.h"

extern bool math_ceil(int argCount, Value* args);
extern bool math_floor(int argCount, Value* args);
extern bool math_fabs(int argCount, Value* args);
extern bool math_exp(int argCount, Value* args);
extern bool math_sqrt(int argCount, Value* args);
extern bool math_sin(int argCount, Value* args);
extern bool math_cos(int argCount, Value* args);
extern bool math_tan(int argCount, Value* args);
extern bool math_atan(int argCount, Value* args);
extern bool math_pow(int argCount, Value* args);
extern bool math_atan2(int argCount, Value* args);
extern bool math_deg(int argCount, Value* args);
extern bool math_rad(int argCount, Value* args);
extern bool math_clamp(int argCount, Value* args);
extern bool math_lerp(int argCount, Value* args);
extern bool math_map(int argCount, Value* args);
extern bool math_norm(int argCount, Value* args);
extern bool math_seed(int argCount, Value* args);
extern bool math_rand(int argCount, Value* args);
extern bool math_round(int argCount, Value* args);
extern void mathCallback(Table* table);

#endif
