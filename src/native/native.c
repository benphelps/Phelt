#include "native/native.h"

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
    { "print", system_print },
    { "sprint", system_sprint },
    { "println", system_println },
    { "module", system_module },
    { "typeof", system_typeof },
    { NULL, NULL },
};

NativeFnEntry systemFns[] = {
    { "env", system_env },
    { "exit", system_exit },
    { "time", system_time },
    { "mtime", system_mtime },
    { "clock", system_clock },
    { "sleep", system_sleep },
    { "usleep", system_usleep },
    { NULL, NULL },
};

NativeFnEntry mathFns[] = {
    { "ceil", math_ceil },
    { "floor", math_floor },
    { "abs", math_fabs },
    { "exp", math_exp },
    { "sqrt", math_sqrt },
    { "sin", math_sin },
    { "cos", math_cos },
    { "tan", math_tan },
    { "atan", math_atan },
    { "pow", math_pow },
    { "atan2", math_atan2 },
    { "deg", math_deg },
    { "rad", math_rad },
    { "clamp", math_clamp },
    { "lerp", math_lerp },
    { "map", math_map },
    { "norm", math_norm },
    { "seed", math_seed },
    { "rand", math_rand },
    { NULL, NULL },
};

NativeFnEntry fileFns[] = {
    { "open", file_open },
    { "tmpfile", file_tmpfile },
    { "mkstemps", file_mkstemps },
    { "close", file_close },
    { "write", file_write },
    { "read", file_read },
    { "seek", file_seek },
    { "tell", file_tell },
    { "flush", file_flush },
    { "getc", file_getc },
    { "gets", file_gets },
    { "puts", file_puts },
    { "putc", file_putc },
    { "remove", file_remove },
    { "rename", file_rename },
    { NULL, NULL },
};

NativeFnEntry httpFns[] = {
    { "get", http_get },
    { "post", http_post },
    { "put", http_put },
    { "patch", http_patch },
    { "delete", http_delete },
    { "head", http_head },
    { "options", http_options },
    { NULL, NULL },
};

NativeFnEntry arrayFns[] = {
    { "length", array_length },
    { "push", array_push },
    { "pop", array_pop },
    { "insert", array_insert },
    { "remove", array_remove },
    { "sort", array_sort },
    { "reverse", array_reverse },
    { "find", array_find },
    { "findLast", array_findLast },
    { "map", array_map },
    { "filter", array_filter },
    { "reduce", array_reduce },
    { "flatten", array_flatten },
    { NULL, NULL },
};

NativeFnEntry tableFns[] = {
    { "length", table_length },
    { "keys", table_keys },
    { "values", table_values },
    { "hasKey", table_hasKey },
    { "remove", table_remove },
    { "insert", table_insert },
    { NULL, NULL },
};

NativeFnEntry stringFns[] = {
    { "length", string_length },
    { "sub", string_sub },
    { "find", string_find },
    { "replace", string_replace },
    { "split", string_split },
    { "trim", string_trim },
    { "upper", string_upper },
    { "lower", string_lower },
    { "reverse", string_reverse },
    { "repeat", string_repeat },
    { NULL, NULL },
};

NativeFnEntry debugFns[] = {
    { "frame", debug_frame },
    { NULL, NULL },
};

// modules
NativeModuleEntry nativeModules[] = {
    { "system", systemFns },
    { "math", mathFns },
    { "file", fileFns },
    { "http", httpFns },
    { "array", arrayFns },
    { "table", tableFns },
    { "string", stringFns },
    { "debug", debugFns },
    { NULL, NULL },
};

NativeModuleCallback nativeModuleCallbacks[] = {
    { "math", mathCallback },
    { "file", fileCallback },
    { NULL, NULL },
};
