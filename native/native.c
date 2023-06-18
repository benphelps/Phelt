#include "native.h"

NativeFnEntry nativeFns[] = {
    // system
    { "time", _time },
    { "clock", _clock },
    { "sleep", _sleep },
    { "usleep", _usleep },
    { "print", _print },
    { "sprint", _sprint },
    { "println", _println },

    // math
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
    { NULL, NULL }, // End of array sentinel
};
