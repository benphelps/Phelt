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

static Value _print(int argCount, Value* args)
{
    for (int i = 0; i < argCount; i++) {
        printValue(args[i]);
        printf(" ");
    }
    printf("\n");
    return NIL_VAL;
}

static Value _printf(int argCount, Value* args)
{
    if (argCount < 1) {
        runtimeError("Expected at least 1 argument but got %d.", argCount);
        return NIL_VAL;
    }

    if (!IS_STRING(args[0])) {
        runtimeError("First argument must be a string.");
        return NIL_VAL;
    }

    ObjString* format = AS_STRING(args[0]);
    int        i      = 1;
    int        j      = 0;
    while (format->chars[j] != '\0') {
        if (format->chars[j] == '%') {
            j++;
            switch (format->chars[j]) {
            case 'd':
            case 'i':
                if (i >= argCount) {
                    runtimeError("Not enough arguments to printf.");
                    return NIL_VAL;
                }
                if (!IS_NUMBER(args[i])) {
                    runtimeError("Argument %i must be a number.", i);
                    return NIL_VAL;
                }
                printf("%i", (int)AS_NUMBER(args[i]));
                i++;
                break;
            case 'f':
                if (i >= argCount) {
                    runtimeError("Not enough arguments to printf.");
                    return NIL_VAL;
                }
                if (!IS_NUMBER(args[i])) {
                    runtimeError("Argument %d must be a number.", i);
                    return NIL_VAL;
                }
                printf("%f", AS_NUMBER(args[i]));
                i++;
                break;
            case 's':
                if (i >= argCount) {
                    runtimeError("Not enough arguments to printf.");
                    return NIL_VAL;
                }
                if (!IS_STRING(args[i])) {
                    runtimeError("Argument %d must be a string.", i);
                    return NIL_VAL;
                }
                printf("%s", AS_CSTRING(args[i]));
                i++;
                break;
            case '%':
                printf("%%");
                break;
            default:
                runtimeError("Invalid format specifier.");
                return NIL_VAL;
            }
        } else {
            printf("%c", format->chars[j]);
        }
        j++;
    }
    printf("\n");
    return NIL_VAL;
}

NativeFnEntry nativeFns[] = {
    { "time", _time },
    { "sleep", _sleep },
    { "print", _print },
    { "printf", _printf },
    { NULL, NULL }, // End of array sentinel
};
