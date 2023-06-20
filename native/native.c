#include "native.h"

NativeFnEntry globalFns[] = {
    { "print", _print },
    { "sprint", _sprint },
    { "println", _println },
    { NULL, NULL },
};

NativeFnEntry systemFns[] = {
    { "time", _time },
    { "clock", _clock },
    { "sleep", _sleep },
    { "usleep", _usleep },
    { NULL, NULL },
};

NativeFnEntry mathFns[] = {
    { "ceil", _ceil },
    { "floor", _floor },
    { "abs", _abs },
    { "exp", _exp },
    { "sqrt", _sqrt },
    { "sin", _sin },
    { "cos", _cos },
    { "tan", _tan },
    { "atan", _atan },
    { "pow", _pow },
    { "atan2", _atan2 },
    { NULL, NULL },
};

// modules
NativeModuleEntry nativeModules[] = {
    { "System", systemFns },
    { "Math", mathFns },
    { NULL, NULL },
};
