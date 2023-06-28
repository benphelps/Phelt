#include "native/math.h"
#include "vm.h"
#include <math.h>

#define DEFINE_MATH_FUNC_SINGLE(FUNC_NAME, TYPE) \
    bool _##FUNC_NAME(int argCount, Value* args) \
    {                                            \
        phelt_checkArgs(1);                        \
        phelt_checkNumber(0);                      \
                                                 \
        TYPE num = (TYPE)phelt_toNumber(0);        \
        phelt_pushNumber(-1, FUNC_NAME(num));      \
        return true;                             \
    }

#define DEFINE_MATH_FUNC_DOUBLE(FUNC_NAME, TYPE)   \
    bool _##FUNC_NAME(int argCount, Value* args)   \
    {                                              \
        phelt_checkArgs(2);                          \
                                                   \
        phelt_checkNumber(0);                        \
        phelt_checkNumber(1);                        \
                                                   \
        TYPE num1 = (TYPE)phelt_toNumber(0);         \
        TYPE num2 = (TYPE)phelt_toNumber(1);         \
        phelt_pushNumber(-1, FUNC_NAME(num1, num2)); \
        return true;                               \
    }

DEFINE_MATH_FUNC_SINGLE(ceil, double)
DEFINE_MATH_FUNC_SINGLE(floor, double)
DEFINE_MATH_FUNC_SINGLE(fabs, double)
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

double deg(double radians)
{
    return radians * (180.0 / M_PI);
}

bool _deg(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);

    double num = phelt_toNumber(0);
    phelt_pushNumber(-1, deg(num));
    return true;
}

double rad(double degrees)
{
    return degrees * (M_PI / 180.0);
}

bool _rad(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);

    double num = phelt_toNumber(0);
    phelt_pushNumber(-1, rad(num));
    return true;
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

bool _clamp(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkNumber(0);
    phelt_checkNumber(1);
    phelt_checkNumber(2);

    double value = phelt_toNumber(0);
    double min   = phelt_toNumber(1);
    double max   = phelt_toNumber(2);
    phelt_pushNumber(-1, clamp(value, min, max));
    return true;
}

double lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

bool _lerp(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkNumber(0);
    phelt_checkNumber(1);
    phelt_checkNumber(2);

    double a = phelt_toNumber(0);
    double b = phelt_toNumber(1);
    double t = phelt_toNumber(2);
    phelt_pushNumber(-1, lerp(a, b, t));
    return true;
}

double map(double value, double start1, double stop1, double start2, double stop2)
{
    return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
}

bool _map(int argCount, Value* args)
{
    phelt_checkArgs(5);
    phelt_checkNumber(0);
    phelt_checkNumber(1);
    phelt_checkNumber(2);
    phelt_checkNumber(3);
    phelt_checkNumber(4);

    double value  = phelt_toNumber(0);
    double start1 = phelt_toNumber(1);
    double stop1  = phelt_toNumber(2);
    double start2 = phelt_toNumber(3);
    double stop2  = phelt_toNumber(4);
    phelt_pushNumber(-1, map(value, start1, stop1, start2, stop2));
    return true;
}

double norm(double value, double start, double stop)
{
    return (value - start) / (stop - start);
}

bool _norm(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkNumber(0);
    phelt_checkNumber(1);
    phelt_checkNumber(2);

    double value = phelt_toNumber(0);
    double start = phelt_toNumber(1);
    double stop  = phelt_toNumber(2);
    phelt_pushNumber(-1, norm(value, start, stop));
    return true;
}

bool _seed(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);
    srand((unsigned int)phelt_toNumber(0));
    return true;
}

bool _rand(int argCount, Value* args)
{
    phelt_checkArgs(0);
    double num = (double)rand() / (double)RAND_MAX;
    phelt_pushNumber(-1, num);
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
    SET_CONST("TAU", M_PI * 2);
    SET_CONST("SQRT2", M_SQRT2);
    SET_CONST("SQRT1_2", M_SQRT1_2);
#undef SET_CONST
}
