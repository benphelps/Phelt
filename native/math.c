#include "native.h"
#include <math.h>

#define DEFINE_MATH_FUNC_SINGLE(FUNC_NAME)                                                 \
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
        double num = AS_NUMBER(args[0]);                                                   \
        return NUMBER_VAL(FUNC_NAME(num));                                                 \
    }

#define DEFINE_MATH_FUNC_DOUBLE(FUNC_NAME)                                                  \
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
        double num1 = AS_NUMBER(args[0]);                                                   \
        double num2 = AS_NUMBER(args[1]);                                                   \
        return NUMBER_VAL(FUNC_NAME(num1, num2));                                           \
    }

DEFINE_MATH_FUNC_SINGLE(ceil)
DEFINE_MATH_FUNC_SINGLE(floor)
DEFINE_MATH_FUNC_SINGLE(abs)
DEFINE_MATH_FUNC_SINGLE(exp)
DEFINE_MATH_FUNC_SINGLE(sqrt)
DEFINE_MATH_FUNC_SINGLE(sin)
DEFINE_MATH_FUNC_SINGLE(cos)
DEFINE_MATH_FUNC_SINGLE(tan)
DEFINE_MATH_FUNC_SINGLE(atan)
DEFINE_MATH_FUNC_SINGLE(asin)
DEFINE_MATH_FUNC_SINGLE(acos)
DEFINE_MATH_FUNC_DOUBLE(pow)
DEFINE_MATH_FUNC_DOUBLE(atan2)
