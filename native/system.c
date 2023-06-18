#include "native.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

double time_in_mill()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    double elapsed = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
    return elapsed;
}

Value _time(int argCount, Value* args)
{
    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return NIL_VAL;
    }

    return NUMBER_VAL(time_in_mill());
}

Value _clock(int argCount, Value* args)
{
    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return NIL_VAL;
    }

    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value _sleep(int argCount, Value* args)
{
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
        return NIL_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
        return NIL_VAL;
    }

    sleep((unsigned int)AS_NUMBER(args[0]));
    return NIL_VAL;
}

Value _usleep(int argCount, Value* args)
{
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
        return NIL_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
        return NIL_VAL;
    }

    usleep((unsigned int)AS_NUMBER(args[0]));
    return NIL_VAL;
}

#define TEMPLATE_BUFFER 1024

// This is your placeholder replacement function
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

// This is your main printing function
Value _print(int argCount, Value* args)
{
    if (argCount < 1) {
        printf("Error: No template provided.\n");
        return NIL_VAL;
    }

    // copy the string template, Values are interned
    char template[TEMPLATE_BUFFER];
    strcpy(template, stringValue(args[0]));

    for (int i = 1; i < argCount; i++) {
        char* value = stringValue(args[i]);
        replace_placeholder(template, value);
    }

    printf("%s", template);

    return NIL_VAL;
}

Value _sprint(int argCount, Value* args)
{
    if (argCount < 1) {
        printf("Error: No template provided.\n");
        return NIL_VAL;
    }

    // copy the string template, Values are interned
    char template[TEMPLATE_BUFFER];
    strcpy(template, stringValue(args[0]));

    for (int i = 1; i < argCount; i++) {
        char* value = stringValue(args[i]);
        replace_placeholder(template, value);
    }

    return OBJ_VAL(copyString(template, strlen(template)));
}

Value _println(int argCount, Value* args)
{
    _print(argCount, args);
    printf("\n");
    return NIL_VAL;
}
