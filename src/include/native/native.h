#ifndef PHELT_NATIVE_H
#define PHELT_NATIVE_H

#include "common.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#include "native/array.h"
#include "native/debug.h"
#include "native/file.h"
#include "native/http.h"
#include "native/math.h"
#include "native/string.h"
#include "native/system.h"
#include "native/table.h"

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

#define phelt_value(pos) (args[pos])

#define phelt_push(pos, val) (args[pos] = val)
#define phelt_pushValue(pos, val) (args[pos] = val)
#define phelt_pushObject(pos, val) (args[pos] = OBJ_VAL(val))
#define phelt_pushCString(pos, val) (args[pos] = OBJ_VAL(copyString(val, strlen(val))))
#define phelt_pushString(pos, val) (args[pos] = OBJ_VAL(val))
#define phelt_pushNumber(pos, val) (args[pos] = NUMBER_VAL(val))
#define phelt_pushBool(pos, val) (args[pos] = BOOL_VAL(val))
#define phelt_pushPointer(pos, val) (args[pos] = POINTER_VAL(val))
#define phelt_pushNil(pos) (args[pos] = NIL_VAL)

#define phelt_isString(pos) (IS_STRING(args[pos]))
#define phelt_isPointer(pos) (IS_POINTER(args[pos]))
#define phelt_isNumber(pos) (IS_NUMBER(args[pos]))
#define phelt_isBool(pos) (IS_BOOL(args[pos]))
#define phelt_isNil(pos) (IS_NIL(args[pos]))
#define phelt_isObject(pos) (IS_OBJ(args[pos]))
#define phelt_isArray(pos) (IS_ARRAY(args[pos]))
#define phelt_isTable(pos) (IS_TABLE(args[pos]))
#define phelt_isFunction(pos) (IS_FUNCTION(args[pos]))
#define phelt_isClosure(pos) (IS_CLOSURE(args[pos]))

#define phelt_toString(val) (AS_STRING(args[val]))
#define phelt_toCString(val) (AS_CSTRING(args[val]))
#define phelt_toNumber(val) (AS_NUMBER(args[val]))
#define phelt_toBool(val) (AS_BOOL(args[val]))
#define phelt_toPointer(val) (AS_POINTER(args[val]))
#define phelt_toObject(val) (AS_OBJ(args[val]))
#define phelt_toArray(val) (AS_ARRAY(args[val]))
#define phelt_toTable(val) (AS_TABLE(args[val]))
#define phelt_toFunction(val) (AS_FUNCTION(args[val]))
#define phelt_toClosure(val) (AS_CLOSURE(args[val]))

#define phelt_objectValue(val) (OBJ_VAL(args[val]))

#define phelt_checkNumber(pos)                                                         \
    if (!phelt_isNumber(pos)) {                                                        \
        phelt_pushObject(-1, formatString("Argument %d must be a pointer.", pos + 1)); \
        return false;                                                                  \
    }

#define phelt_checkString(pos)                                                        \
    if (!phelt_isString(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a string.", pos + 1)); \
        return false;                                                                 \
    }

#define phelt_checkPointer(pos)                                                        \
    if (!phelt_isPointer(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a pointer.", pos + 1)); \
        return false;                                                                  \
    }

#define phelt_checkObject(pos)                                                        \
    if (!phelt_isObject(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a object.", pos + 1)); \
        return false;                                                                 \
    }

#define phelt_checkBool(pos)                                                           \
    if (!phelt_isBool(pos)) {                                                          \
        phelt_pushObject(-1, formatString("Argument %d must be a boolean.", pos + 1)); \
        return false;                                                                  \
    }

#define phelt_checkNil(pos)                                                        \
    if (!phelt_isNil(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a nil.", pos + 1)); \
        return false;                                                              \
    }

#define phelt_checkArray(pos)                                                        \
    if (!phelt_isArray(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a array.", pos + 1)); \
        return false;                                                                \
    }

#define phelt_checkTable(pos)                                                        \
    if (!phelt_isTable(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a table.", pos + 1)); \
        return false;                                                                \
    }

#define phelt_checkFunction(pos)                                                        \
    if (!phelt_isFunction(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a function.", pos + 1)); \
        return false;                                                                   \
    }

#define phelt_checkClosure(pos)                                                        \
    if (!phelt_isClosure(pos)) {                                                       \
        phelt_pushObject(-1, formatString("Argument %d must be a closure.", pos + 1)); \
        return false;                                                                  \
    }

#define phelt_checkArgs(count)                                                                    \
    if (argCount != count) {                                                                      \
        phelt_pushObject(-1, formatString("Expected %d arguments but got %d.", count, argCount)); \
        return false;                                                                             \
    }

#define phelt_checkMinArgs(count)                                                                 \
    if (argCount < count) {                                                                       \
        phelt_pushObject(-1, formatString("Expected %d arguments but got %d.", count, argCount)); \
        return false;                                                                             \
    }

#define phelt_error(msg, ...) phelt_pushObject(-1, formatString(msg, ##__VA_ARGS__))

#define phelt_callClosure(closure, args)                        \
    do {                                                        \
        Chunk chunk                 = closure->function->chunk; \
        chunk.code[chunk.count - 1] = OP_REENTER;               \
        call(closure, args);                                    \
        run();                                                  \
        *(vm.stackTop - 1 - args) = *(vm.stackTop - 1);         \
    } while (false)

#endif
