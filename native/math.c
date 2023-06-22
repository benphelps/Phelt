#include "math.h"
#include <math.h>

double deg(double radians)
{
    return radians * (180.0 / M_PI);
}

double rad(double degrees)
{
    return degrees * (M_PI / 180.0);
}

double clamp(double value, double min, double max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

double lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

double map(double value, double start1, double stop1, double start2, double stop2)
{
    return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
}

double norm(double value, double start, double stop)
{
    return (value - start) / (stop - start);
}

#define DEFINE_MATH_FUNC_SINGLE(FUNC_NAME, TYPE)                                                  \
    bool _##FUNC_NAME(int argCount, Value* args)                                                  \
    {                                                                                             \
        if (argCount != 1) {                                                                      \
            lux_pushObject(-1, formatString(#FUNC_NAME "() takes 1 argument, got %d", argCount)); \
            return false;                                                                         \
        }                                                                                         \
                                                                                                  \
        if (!lux_isNumber(0)) {                                                                   \
            lux_pushObject(-1, formatString(#FUNC_NAME "() argument must be a number"));          \
            return false;                                                                         \
        }                                                                                         \
                                                                                                  \
        TYPE num = (TYPE)lux_toNumber(0);                                                         \
        lux_pushNumber(-1, FUNC_NAME(num));                                                       \
        return true;                                                                              \
    }

#define DEFINE_MATH_FUNC_DOUBLE(FUNC_NAME, TYPE)                                                   \
    bool _##FUNC_NAME(int argCount, Value* args)                                                   \
    {                                                                                              \
        if (argCount != 2) {                                                                       \
            lux_pushObject(-1, formatString(#FUNC_NAME "() takes 2 arguments, got %d", argCount)); \
            return false;                                                                          \
        }                                                                                          \
                                                                                                   \
        if (!lux_isNumber(0) || !lux_isNumber(1)) {                                                \
            lux_pushObject(-1, formatString(#FUNC_NAME "() arguments must be numbers"));           \
            return false;                                                                          \
        }                                                                                          \
                                                                                                   \
        TYPE num1 = (TYPE)lux_toNumber(0);                                                         \
        TYPE num2 = (TYPE)lux_toNumber(1);                                                         \
        lux_pushNumber(-1, FUNC_NAME(num1, num2));                                                 \
        return true;                                                                               \
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

bool _deg(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("deg() takes 1 argument, got %d", argCount));
        return false;
    }

    if (!lux_isNumber(0)) {
        lux_pushObject(-1, formatString("deg() argument must be a number"));
        return false;
    }

    double num = lux_toNumber(0);
    lux_pushNumber(-1, deg(num));
    return true;
}

bool _rad(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("rad() takes 1 argument, got %d", argCount));
        return false;
    }

    if (!lux_isNumber(0)) {
        lux_pushObject(-1, formatString("rad() argument must be a number"));
        return false;
    }

    double num = lux_toNumber(0);
    lux_pushNumber(-1, rad(num));
    return true;
}

bool _clamp(int argCount, Value* args)
{
    if (argCount != 3) {
        lux_pushObject(-1, formatString("clamp() takes 3 arguments, got %d", argCount));
        return false;
    }

    if (!lux_isNumber(0) || !lux_isNumber(1) || !lux_isNumber(2)) {
        lux_pushObject(-1, formatString("clamp() arguments must be numbers"));
        return false;
    }

    double value = lux_toNumber(0);
    double min   = lux_toNumber(1);
    double max   = lux_toNumber(2);
    lux_pushNumber(-1, clamp(value, min, max));
    return true;
}

bool _lerp(int argCount, Value* args)
{
    if (argCount != 3) {
        lux_pushObject(-1, formatString("lerp() takes 3 arguments, got %d", argCount));
        return false;
    }

    if (!lux_isNumber(0) || !lux_isNumber(1) || !lux_isNumber(2)) {
        lux_pushObject(-1, formatString("lerp() arguments must be numbers"));
        return false;
    }

    double a = lux_toNumber(0);
    double b = lux_toNumber(1);
    double t = lux_toNumber(2);
    lux_pushNumber(-1, lerp(a, b, t));
    return true;
}

bool _map(int argCount, Value* args)
{
    if (argCount != 5) {
        lux_pushObject(-1, formatString("map() takes 5 arguments, got %d", argCount));
        return false;
    }

    if (!lux_isNumber(0) || !lux_isNumber(1) || !lux_isNumber(2) || !lux_isNumber(3) || !lux_isNumber(4)) {
        lux_pushObject(-1, formatString("map() arguments must be numbers"));
        return false;
    }

    double value  = lux_toNumber(0);
    double start1 = lux_toNumber(1);
    double stop1  = lux_toNumber(2);
    double start2 = lux_toNumber(3);
    double stop2  = lux_toNumber(4);
    lux_pushNumber(-1, map(value, start1, stop1, start2, stop2));
    return true;
}

bool _norm(int argCount, Value* args)
{
    if (argCount != 3) {
        lux_pushObject(-1, formatString("norm() takes 3 arguments, got %d", argCount));
        return false;
    }

    if (!lux_isNumber(0) || !lux_isNumber(1) || !lux_isNumber(2)) {
        lux_pushObject(-1, formatString("norm() arguments must be numbers"));
        return false;
    }

    double value = lux_toNumber(0);
    double start = lux_toNumber(1);
    double stop  = lux_toNumber(2);
    lux_pushNumber(-1, norm(value, start, stop));
    return true;
}

void mathCallback(Table* table)
{
#define SET_CONST(name, value)                     \
    push(OBJ_VAL(copyString(name, strlen(name)))); \
    push(NUMBER_VAL(value));                       \
    tableSet(table, vm.stack[0], vm.stack[1]);     \
    pop();                                         \
    pop();

    SET_CONST("E", M_E);
    SET_CONST("LOG2E", M_LOG2E);
    SET_CONST("LOG10E", M_LOG10E);
    SET_CONST("LN2", M_LN2);
    SET_CONST("LN10", M_LN10);
    SET_CONST("PI", M_PI);
    SET_CONST("PI_2", M_PI_2);
    SET_CONST("PI_4", M_PI_4);
    SET_CONST("1_PI", M_1_PI);
    SET_CONST("2_PI", M_2_PI);
    SET_CONST("2_SQRTPI", M_2_SQRTPI);
    SET_CONST("SQRT2", M_SQRT2);
    SET_CONST("SQRT1_2", M_SQRT1_2);
#undef SET_CONST
}
