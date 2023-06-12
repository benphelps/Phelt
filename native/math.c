#include "native.h"
#include <math.h>

#define DEFINE_MATH_FUNC_SINGLE(FUNC_NAME, TYPE)                                           \
    Value _##FUNC_NAME(int argCount, Value* args)                                          \
    {                                                                                      \
        if (argCount != 1) {                                                               \
            runtimeError(#FUNC_NAME "() takes exactly one argument (%d given)", argCount); \
            return NIL_VAL;                                                                \
        }                                                                                  \
                                                                                           \
        if (!IS_NUMBER(args[0])) {                                                         \
            runtimeError(#FUNC_NAME "() argument must be a number");                       \
            return NIL_VAL;                                                                \
        }                                                                                  \
                                                                                           \
        TYPE num = (TYPE)AS_NUMBER(args[0]);                                               \
        return NUMBER_VAL(FUNC_NAME(num));                                                 \
    }

#define DEFINE_MATH_FUNC_DOUBLE(FUNC_NAME, TYPE)                                            \
    Value _##FUNC_NAME(int argCount, Value* args)                                           \
    {                                                                                       \
        if (argCount != 2) {                                                                \
            runtimeError(#FUNC_NAME "() takes exactly two arguments (%d given)", argCount); \
            return NIL_VAL;                                                                 \
        }                                                                                   \
                                                                                            \
        if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {                                   \
            runtimeError(#FUNC_NAME "() arguments must be numbers");                        \
            return NIL_VAL;                                                                 \
        }                                                                                   \
                                                                                            \
        TYPE num1 = (TYPE)AS_NUMBER(args[0]);                                               \
        TYPE num2 = (TYPE)AS_NUMBER(args[1]);                                               \
        return NUMBER_VAL(FUNC_NAME(num1, num2));                                           \
    }

DEFINE_MATH_FUNC_SINGLE(ceil, double)
DEFINE_MATH_FUNC_SINGLE(floor, double)
DEFINE_MATH_FUNC_SINGLE(abs, int)
DEFINE_MATH_FUNC_SINGLE(exp, double)
DEFINE_MATH_FUNC_SINGLE(sqrt, double)
DEFINE_MATH_FUNC_SINGLE(sin, double)
DEFINE_MATH_FUNC_SINGLE(cos, double)
DEFINE_MATH_FUNC_SINGLE(tan, double)
DEFINE_MATH_FUNC_SINGLE(atan, double)
DEFINE_MATH_FUNC_SINGLE(asin, double)
DEFINE_MATH_FUNC_SINGLE(acos, double)
DEFINE_MATH_FUNC_DOUBLE(pow, double)
DEFINE_MATH_FUNC_DOUBLE(atan2, double)
