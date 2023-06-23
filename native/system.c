#include "system.h"
#include "../value.h"
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

double time_in_mill()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    double elapsed = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
    return elapsed;
}

bool _time(int argCount, Value* args)
{
    if (argCount != 0) {
        lux_pushObject(-1, formatString("Expected 0 arguments but got %d.", argCount));
        return false;
    }

    lux_pushNumber(-1, time_in_mill());
    return true;
}

bool _clock(int argCount, Value* args)
{
    if (argCount != 0) {
        lux_pushObject(-1, formatString("Expected 0 arguments but got %d.", argCount));
        return false;
    }

    lux_pushNumber(-1, (double)clock() / CLOCKS_PER_SEC);
    return true;
}

bool _sleep(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isNumber(0)) {
        lux_pushObject(-1, formatString("Argument must be a number."));
        return false;
    }

    sleep((unsigned int)lux_toNumber(0));
    lux_pushNil(-1);
    return true;
}

bool _usleep(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isNumber(0)) {
        lux_pushObject(-1, formatString("Argument must be a number."));
        return false;
    }

    usleep((unsigned int)lux_toNumber(0));
    lux_pushNil(-1);
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
        printf("Error: No template provided.\n");
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
        printf("Error: No template provided.\n");
        return false;
    }

    char template[TEMPLATE_BUFFER];
    strcpy(template, stringValue(args[0]));

    for (int i = 1; i < argCount; i++) {
        char* value = stringValue(args[i]);
        replace_placeholder(template, value);
    }

    lux_pushObject(-1, copyString(template, strlen(template)));
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
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isObject(0)) {
        lux_pushObject(-1, formatString("Argument must be an object."));
        return false;
    }

    int length = objectLength(lux_objectValue(0));

    if (length == -1) {
        lux_pushObject(-1, formatString("Argument must be an object with a length."));
        return false;
    }

    lux_pushNumber(-1, length);
    return true;
}
