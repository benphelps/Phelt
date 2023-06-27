#include "native.h"

NativeModuleEntry* findNativeModule(NativeModuleEntry* modules, const char* name)
{
    NativeModuleEntry* module = modules;
    while (module->name != NULL) {
        if (strcmp(module->name, name) == 0) {
            return module;
        }
        ++module;
    }
    return NULL;
}

NativeModuleCallback* findNativeModuleCallback(NativeModuleCallback* callbacks, const char* name)
{
    NativeModuleCallback* callback = callbacks;
    while (callback->name != NULL) {
        if (strcmp(callback->name, name) == 0) {
            return callback;
        }
        ++callback;
    }
    return NULL;
}

ObjTable* defineNativeModule(NativeModuleEntry* module)
{
    ObjTable* table = newTable();

    for (NativeFnEntry* entry = module->fns; entry->name != NULL; entry++) {
        tableSet(&table->table, OBJ_VAL(copyString(entry->name, (int)strlen(entry->name))), OBJ_VAL(newNative(entry->function)));
    }

    NativeModuleCallback* callback = findNativeModuleCallback(nativeModuleCallbacks, module->name);
    if (callback != NULL)
        callback->callback(&table->table);

    return table;
}

NativeFnEntry globalFns[] = {
    { "print", _print },
    { "sprint", _sprint },
    { "println", _println },
    { "len", _len },
    { "module", _module },
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
    { "abs", _fabs },
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
