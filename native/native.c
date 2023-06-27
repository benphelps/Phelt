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
    // { "call", _call },
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

NativeFnEntry httpFns[] = {
    { "get", _get },
    { "post", _post },
    { "put", _put },
    { "patch", _patch },
    { "delete", _delete },
    { "head", _head },
    { "options", _options },
    { NULL, NULL },
};

// modules
NativeModuleEntry nativeModules[] = {
    { "system", systemFns },
    { "math", mathFns },
    { "file", fileFns },
    { "http", httpFns },
    { NULL, NULL },
};

NativeModuleCallback nativeModuleCallbacks[] = {
    { "math", mathCallback },
    { "file", fileCallback },
    { NULL, NULL },
};
