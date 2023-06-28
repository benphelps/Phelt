#ifndef LUX_NATIVE_H
#define LUX_NATIVE_H

#include "common.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#include "native/file.h"
#include "native/http.h"
#include "native/math.h"
#include "native/system.h"

typedef struct {
    const char* name;
    NativeFn    function;
} NativeFnEntry;

typedef struct {
    const char*    name;
    NativeFnEntry* fns;
} NativeModuleEntry;

typedef struct {
    const char* name;
    void (*callback)(Table*);
} NativeModuleCallback;

extern NativeFnEntry        globalFns[];
extern NativeModuleEntry    nativeModules[];
extern NativeModuleCallback nativeModuleCallbacks[];

NativeModuleEntry*    findNativeModule(NativeModuleEntry* modules, const char* name);
NativeModuleCallback* findNativeModuleCallback(NativeModuleCallback* callbacks, const char* name);
ObjTable*             defineNativeModule(NativeModuleEntry* module);

#define lux_push(pos, val) (args[pos] = val)
#define lux_pushObject(pos, val) (args[pos] = OBJ_VAL(val))
#define lux_pushCString(pos, val) (args[pos] = OBJ_VAL(copyString(val, strlen(val))))
#define lux_pushString(pos, val) (args[pos] = OBJ_VAL(val))
#define lux_pushNumber(pos, val) (args[pos] = NUMBER_VAL(val))
#define lux_pushBool(pos, val) (args[pos] = BOOL_VAL(val))
#define lux_pushPointer(pos, val) (args[pos] = POINTER_VAL(val))
#define lux_pushNil(pos) (args[pos] = NIL_VAL)

#define lux_isString(pos) (IS_STRING(args[pos]))
#define lux_isPointer(pos) (IS_POINTER(args[pos]))
#define lux_isNumber(pos) (IS_NUMBER(args[pos]))
#define lux_isBool(pos) (IS_BOOL(args[pos]))
#define lux_isNil(pos) (IS_NIL(args[pos]))
#define lux_isObject(pos) (IS_OBJ(args[pos]))

#define lux_toString(val) (AS_STRING(args[val]))
#define lux_toCString(val) (AS_CSTRING(args[val]))
#define lux_toNumber(val) (AS_NUMBER(args[val]))
#define lux_toBool(val) (AS_BOOL(args[val]))
#define lux_toPointer(val) (AS_POINTER(args[val]))
#define lux_toObject(val) (AS_OBJ(args[val]))
#define lux_toArray(val) (AS_ARRAY(args[val]))
#define lux_toTable(val) (AS_TABLE(args[val]))
#define lux_toFunction(val) (AS_FUNCTION(args[val]))
#define lux_toClosure(val) (AS_CLOSURE(args[val]))

#define lux_objectValue(val) (OBJ_VAL(args[val]))

#define lux_checkNumber(pos)                                                         \
    if (!lux_isNumber(pos)) {                                                        \
        lux_pushObject(-1, formatString("Argument %d must be a pointer.", pos + 1)); \
        return false;                                                                \
    }

#define lux_checkString(pos)                                                        \
    if (!lux_isString(pos)) {                                                       \
        lux_pushObject(-1, formatString("Argument %d must be a string.", pos + 1)); \
        return false;                                                               \
    }

#define lux_checkPointer(pos)                                                        \
    if (!lux_isPointer(pos)) {                                                       \
        lux_pushObject(-1, formatString("Argument %d must be a pointer.", pos + 1)); \
        return false;                                                                \
    }

#define lux_checkObject(pos)                                                        \
    if (!lux_isObject(pos)) {                                                       \
        lux_pushObject(-1, formatString("Argument %d must be a object.", pos + 1)); \
        return false;                                                               \
    }

#define lux_checkBool(pos)                                                           \
    if (!lux_isBool(pos)) {                                                          \
        lux_pushObject(-1, formatString("Argument %d must be a boolean.", pos + 1)); \
        return false;                                                                \
    }

#define lux_checkNil(pos)                                                        \
    if (!lux_isNil(pos)) {                                                       \
        lux_pushObject(-1, formatString("Argument %d must be a nil.", pos + 1)); \
        return false;                                                            \
    }

#define lux_checkArgs(count)                                                                    \
    if (argCount != count) {                                                                    \
        lux_pushObject(-1, formatString("Expected %d arguments but got %d.", count, argCount)); \
        return false;                                                                           \
    }

#define lux_error(msg, ...) lux_pushObject(-1, formatString(msg, ##__VA_ARGS__))

#endif
