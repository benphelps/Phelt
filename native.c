#include <time.h>
#include <unistd.h>

#include "native.h"
#include "vm.h"

static Value _time(int argCount, Value* args)
{
    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return NIL_VAL;
    }

    return NUMBER_VAL((double)time(NULL));
}

static Value _sleep(int argCount, Value* args)
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
static Value _print(int argCount, Value* args)
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

    printf("%s\n", template);

    return NIL_VAL;
}

NativeFnEntry nativeFns[] = {
    { "time", _time },
    { "sleep", _sleep },
    { "print", _print },
    { NULL, NULL }, // End of array sentinel
};
