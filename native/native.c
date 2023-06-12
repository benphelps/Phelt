#include "native.h"

NativeFnEntry nativeFns[] = {
    // system
    { "time", _time },
    { "sleep", _sleep },
    { "print", _print },

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
