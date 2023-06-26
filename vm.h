#ifndef lux_vm_h
#define lux_vm_h

#include "chunk.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "utf8.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure* closure;
    uint8_t*    ip;
    Value*      slots;
} CallFrame;

typedef struct
{
    CallFrame   frames[FRAMES_MAX];
    int         frameCount;
    bool        errorState;
    Value       stack[STACK_MAX];
    Value*      stackTop;
    Table       globals;
    Table       strings;
    ObjUpvalue* openUpvalues;

    ObjString* initString;
    ObjString* strString;
    ObjString* addString;
    ObjString* subString;
    ObjString* mulString;
    ObjString* divString;
    ObjString* gtString;
    ObjString* ltString;
    ObjString* eqString;
    ObjString* andString;
    ObjString* orString;
    ObjString* xorString;
    ObjString* modString;
    ObjString* notString;
    ObjString* rshiftString;
    ObjString* lshiftString;

    size_t bytesAllocated;
    size_t nextGC;

    Obj* objects;

    int   grayCount;
    int   grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void            initVM();
void            freeVM();
InterpretResult interpret(const char* sourcePath, utf8_int8_t* source);
void            push(Value value);
Value           pop();
bool            call(ObjClosure* closure, int argCount);
InterpretResult run(bool reenter);

#endif
