#include "native.h"

NativeFnEntry globalFns[] = {
    { "print", _print },
    { "sprint", _sprint },
    { "println", _println },
    { "len", _len },
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
    { "deg", _deg },
    { "rad", _rad },
    { "clamp", _clamp },
    { "lerp", _lerp },
    { "map", _map },
    { "norm", _norm },
    { NULL, NULL },
};

NativeFnEntry fileFns[] = {
    { "fopen", _fopen },
    { "tmpfile", _tmpfile },
    { "mkstemps", _mkstemps },
    { "fclose", _fclose },
    { "fwrite", _fwrite },
    { "fread", _fread },
    { "fseek", _fseek },
    { "ftell", _ftell },
    { "fflush", _fflush },
    { "fgetc", _fgetc },
    { "fgets", _fgets },
    { "fputs", _fputs },
    { "fputc", _fputc },
    { "remove", _remove },
    { "rename", _rename },
    { NULL, NULL },
};

// modules
NativeModuleEntry nativeModules[] = {
    { "system", systemFns },
    { "math", mathFns },
    { "file", fileFns },
    { NULL, NULL },
};

NativeModuleCallback nativeModuleCallbacks[] = {
    { "math", mathCallback },
    { "file", fileCallback },
    { NULL, NULL },
};
