#ifndef LUX_NATIVE_H
#define LUX_NATIVE_H

#include "../common.h"
#include "../object.h"
#include "../value.h"
#include "../vm.h"

#include "file.h"
#include "math.h"
#include "system.h"

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

#endif
