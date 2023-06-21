#include "file.h"
#include <stdio.h>

// FILE * fopen(const char * restrict path, const char * restrict mode);
// fp = fopen("file.txt", "w");
bool _fopen(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 2 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isString(0) || !lux_isString(1)) {
        lux_pushObject(-1, formatString("Expected string arguments."));
        return false;
    }

    const char* path = lux_toCString(0);
    const char* mode = lux_toCString(1);

    FILE* file = fopen(path, mode);
    if (file == NULL) {
        lux_pushObject(-1, formatString("Failed to open file '%s' with mode '%s'.", path, mode));
        return false;
    }

    lux_pushPointer(-1, file);
    return true;
}

// int fclose(FILE * stream);
// fclose(fp)
bool _fclose(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0)) {
        lux_pushObject(-1, formatString("Expected pointer argument."));
        return false;
    }

    FILE* file   = (FILE*)lux_toPointer(0);
    int   result = fclose(file);
    if (result != 0) {
        lux_pushObject(-1, formatString("Failed to close file."));
        return false;
    }

    return true;
}

// size_t fwrite(const void * restrict ptr, size_t size, size_t nitems, FILE * restrict stream);
// bytes_written = fwrite(fp, str)
bool _fwrite(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 2 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0) || !lux_isString(1)) {
        lux_pushObject(-1, formatString("Expected string, pointer arguments."));
        return false;
    }

    FILE*      stream = (FILE*)lux_toPointer(0);
    ObjString* string = lux_toString(1);

    size_t result = fwrite(string->chars, sizeof(char), string->length, stream);
    if (result != string->length) {
        lux_pushObject(-1, formatString("Failed to write to file."));
        return false;
    }

    lux_pushNumber(-1, result);
    return true;
}

// size_t fread(void * restrict ptr, size_t size, size_t nitems, FILE * restrict stream);
// str = fread(fp, bytes)
bool _fread(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 2 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0) || !lux_isNumber(1)) {
        lux_pushObject(-1, formatString("Expected pointer, number arguments."));
        return false;
    }

    FILE*  stream = (FILE*)lux_toPointer(0);
    size_t bytes  = (size_t)lux_toNumber(1);

    char* buffer = (char*)malloc(bytes);
    if (buffer == NULL) {
        lux_pushObject(-1, formatString("Failed to allocate memory."));
        return false;
    }

    size_t result = fread(buffer, sizeof(char), bytes, stream);
    if (result != bytes) {
        lux_pushObject(-1, formatString("Failed to read from file."));
        return false;
    }

    ObjString* string = copyString(buffer, bytes);

    lux_pushString(-1, string);
    return true;
}
