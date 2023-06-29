#include "native/system.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

Value getEnv(const char* name)
{
    char* env = getenv(name);
    if (env == NULL) {
        return NIL_VAL;
    }
    return OBJ_VAL(copyString(env, strlen(env)));
}

bool _exit(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);

    exit((int)phelt_toNumber(0));
    return true;
}

// double time_in_mill(void);
// let time = system.time()
bool _time(int argCount, Value* args)
{
    phelt_checkArgs(0);

    phelt_pushNumber(-1, time(NULL));
    return true;
}

// double time_in_mill(void);
// let time = system.time()
bool _mtime(int argCount, Value* args)
{
    phelt_checkArgs(0);

    struct timeval time;
    gettimeofday(&time, NULL);
    double elapsed = ((double)time.tv_sec * 1000) + ((double)time.tv_usec / 1000);

    phelt_pushNumber(-1, elapsed);
    return true;
}

// clock_t clock(void);
// let clock = system.clock()
bool _clock(int argCount, Value* args)
{
    phelt_checkArgs(0);

    phelt_pushNumber(-1, (double)clock() / CLOCKS_PER_SEC);
    return true;
}

// int sleep(unsigned int seconds);
// let sleep = system.sleep(1)
bool _sleep(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);

    sleep((unsigned int)phelt_toNumber(0));
    phelt_pushNil(-1);
    return true;
}

// int usleep(useconds_t usec);
// let usleep = system.usleep(1000)
bool _usleep(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);

    usleep((unsigned int)phelt_toNumber(0));
    phelt_pushNil(-1);
    return true;
}

#define TEMPLATE_BUFFER 1024

char* replace_placeholder(char* template, char* value)
{
    char* ptr = strstr(template, "{}");
    if (ptr != NULL) {
        char   buffer[TEMPLATE_BUFFER];
        size_t len = ptr - template;
        strncpy(buffer, template, len);
        buffer[len] = '\0';
        strcat(buffer, value);
        strcat(buffer, ptr + 2);
        strcpy(template, buffer);
    }
    return template;
}

bool _print(int argCount, Value* args)
{
    if (argCount < 1) {
        phelt_error("No template provided.");
        return false;
    }

    char template[TEMPLATE_BUFFER];
    strcpy(template, stringValue(args[0]));

    for (int i = 1; i < argCount; i++) {
        char* value = stringValue(args[i]);
        replace_placeholder(template, value);
    }

    printf("%s", template);

    return true;
}

bool _sprint(int argCount, Value* args)
{
    if (argCount < 1) {
        phelt_error("No template provided.");
        return false;
    }

    char template[TEMPLATE_BUFFER];
    strcpy(template, stringValue(args[0]));

    for (int i = 1; i < argCount; i++) {
        char* value = stringValue(args[i]);
        replace_placeholder(template, value);
    }

    phelt_pushObject(-1, copyString(template, strlen(template)));
    return true;
}

bool _println(int argCount, Value* args)
{
    _print(argCount, args);
    printf("\n");
    return true;
}

bool _len(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkObject(0);

    int length = objectLength(OBJ_VAL(phelt_toObject(0)));

    if (length == -1) {
        phelt_error("Argument must be an object with a length.");
        return false;
    }

    phelt_pushNumber(-1, length);
    return true;
}

bool _module(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    const char*        name  = phelt_toCString(0);
    NativeModuleEntry* entry = findNativeModule(nativeModules, name);

    if (entry == NULL) {
        phelt_error("Module '%s' not found.", name);
        return false;
    }

    ObjTable* table = defineNativeModule(entry);

    phelt_pushObject(-1, table);
    return true;
}

// bool _call(int argCount, Value* args)
// {
//     if (argCount < 1) {
//         phelt_pushObject(-1, formatString("Expected at least 1 argument, got %d.", argCount));
//         return false;
//     }

//     if (!phelt_isObject(0)) {
//         phelt_pushObject(-1, formatString("Argument must be an object."));
//         return false;
//     }

//     ObjClosure* closure = phelt_toClosure(0);
//     // patch the function to reenter the VM, instead of returning
//     closure->function->chunk.code[closure->function->chunk.count - 1] = OP_REENTER;
//     call(closure, argCount - 1);
//     run(true);
//     *(vm.stackTop - 1 - argCount) = *(vm.stackTop - 1);
//     return true;
// }
