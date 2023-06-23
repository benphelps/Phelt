#include "file.h"
#include <stdio.h>

// FILE * fopen(const char * restrict path, const char * restrict mode);
// let fp = fopen("file.txt", "w+");
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

    lux_pushPointer(-1, (uintptr_t)file);
    return true;
}

// FILE * tmpfile(void);
// let fp = file.tmpfile()
bool _tmpfile(int argCount, Value* args)
{
    if (argCount != 0) {
        lux_pushObject(-1, formatString("Expected 0 arguments but got %d.", argCount));
        return false;
    }

    FILE* file = tmpfile();
    if (file == NULL) {
        lux_pushObject(-1, formatString("Failed to create temporary file."));
        return false;
    }

    lux_pushPointer(-1, (uintptr_t)file);
    return true;
}

// int mkstemp(char *template);
// let fp = file.mkstemps("fileXXXXXX")
bool _mkstemps(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isString(0)) {
        lux_pushObject(-1, formatString("Expected string argument."));
        return false;
    }

    char* template = lux_toCString(0);
    int fd         = mkstemp(template);
    if (fd == -1) {
        lux_pushObject(-1, formatString("Failed to create temporary file."));
        return false;
    }

    FILE* file = fdopen(fd, "w+");
    if (file == NULL) {
        lux_pushObject(-1, formatString("Failed to open temporary file."));
        return false;
    }

    lux_pushPointer(-1, (uintptr_t)file);
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
        lux_pushObject(-1, formatString("Expected pointer, string arguments."));
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
// let str = file.fread(fp, bytes)
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
    free(buffer);

    lux_pushString(-1, string);
    return true;
}

// fseek(FILE * stream, long int offset, int whence);
// let pos = file.fseek(fp, 0, SEEK_END)
bool _fseek(int argCount, Value* args)
{
    if (argCount != 3) {
        lux_pushObject(-1, formatString("Expected 3 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0) || !lux_isNumber(1) || !lux_isNumber(2)) {
        lux_pushObject(-1, formatString("Expected pointer, number, number arguments."));
        return false;
    }

    FILE*    stream = (FILE*)lux_toPointer(0);
    long int offset = (long int)lux_toNumber(1);
    int      whence = (int)lux_toNumber(2);

    int result = fseek(stream, offset, whence);
    if (result != 0) {
        lux_pushObject(-1, formatString("Failed to seek file."));
        return false;
    }

    return true;
}

// long int ftell(FILE * stream);
// let pos = file.ftell(fp)
bool _ftell(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0)) {
        lux_pushObject(-1, formatString("Expected pointer argument."));
        return false;
    }

    FILE* stream = (FILE*)lux_toPointer(0);

    long int result = ftell(stream);
    if (result == -1) {
        lux_pushObject(-1, formatString("Failed to get file position."));
        return false;
    }

    lux_pushNumber(-1, result);
    return true;
}

// void rewind(FILE * stream);
// file.rewind(fp)
bool _rewind(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0)) {
        lux_pushObject(-1, formatString("Expected pointer argument."));
        return false;
    }

    FILE* stream = (FILE*)lux_toPointer(0);

    rewind(stream);
    return true;
}

// int fflush(FILE * stream);
// file.fflush(fp)
bool _fflush(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0)) {
        lux_pushObject(-1, formatString("Expected pointer argument."));
        return false;
    }

    FILE* stream = (FILE*)lux_toPointer(0);

    int result = fflush(stream);
    if (result != 0) {
        lux_pushObject(-1, formatString("Failed to flush file."));
        return false;
    }

    return true;
}

// int fgetc(FILE * stream);
// let char = file.fgetc(fp)
bool _fgetc(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0)) {
        lux_pushObject(-1, formatString("Expected pointer argument."));
        return false;
    }

    FILE* stream = (FILE*)lux_toPointer(0);

    int result = fgetc(stream);
    if (result == EOF) {
        lux_pushObject(-1, formatString("Failed to get character."));
        return false;
    }

    lux_pushNumber(-1, result);
    return true;
}

// int fgets(char * str, int num, FILE * stream);
// let str = file.fgets(fp, 10)
bool _fgets(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 3 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0) || !lux_isNumber(1)) {
        lux_pushObject(-1, formatString("Expected pointer, number arguments."));
        return false;
    }

    FILE* stream = (FILE*)lux_toPointer(0);
    int   num    = (int)lux_toNumber(1);
    char  str[num];

    char* result = fgets(str, num, stream);
    if (result == NULL) {
        lux_pushObject(-1, formatString("Failed to get string."));
        return false;
    }

    lux_pushString(-1, copyString((const char*)str, strlen(str)));
    return true;
}

// int fputc(int character, FILE * stream);
// file.fputc(fp, 65)
bool _fputc(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 2 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0) || !lux_isNumber(1)) {
        lux_pushObject(-1, formatString("Expected pointer, number arguments."));
        return false;
    }

    FILE* stream    = (FILE*)lux_toPointer(0);
    int   character = (int)lux_toNumber(1);

    int result = fputc(character, stream);
    if (result == EOF) {
        lux_pushObject(-1, formatString("Failed to put character."));
        return false;
    }

    return true;
}

// int fputs(const char * str, FILE * stream);
// file.fputs(fp, "Hello World")
bool _fputs(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 2 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isPointer(0) || !lux_isString(1)) {
        lux_pushObject(-1, formatString("Expected pointer, string arguments."));
        return false;
    }

    FILE*       stream = (FILE*)lux_toPointer(0);
    const char* str    = lux_toCString(1);

    int result = fputs(str, stream);
    if (result == EOF) {
        lux_pushObject(-1, formatString("Failed to put string."));
        return false;
    }

    return true;
}

// int remove(const char * filename);
// file.remove("test.txt")
bool _remove(int argCount, Value* args)
{
    if (argCount != 1) {
        lux_pushObject(-1, formatString("Expected 1 argument but got %d.", argCount));
        return false;
    }

    if (!lux_isString(0)) {
        lux_pushObject(-1, formatString("Expected string argument."));
        return false;
    }

    const char* filename = lux_toCString(0);

    int result = remove(filename);
    if (result != 0) {
        lux_pushObject(-1, formatString("Failed to remove file."));
        return false;
    }

    return true;
}

// int rename(const char * oldname, const char * newname);
// file.rename("test.txt", "test2.txt")
bool _rename(int argCount, Value* args)
{
    if (argCount != 2) {
        lux_pushObject(-1, formatString("Expected 2 arguments but got %d.", argCount));
        return false;
    }

    if (!lux_isString(0) || !lux_isString(1)) {
        lux_pushObject(-1, formatString("Expected string arguments."));
        return false;
    }

    const char* oldname = lux_toCString(0);
    const char* newname = lux_toCString(1);

    int result = rename(oldname, newname);
    if (result != 0) {
        lux_pushObject(-1, formatString("Failed to rename file."));
        return false;
    }

    return true;
}

void fileCallback(Table* table)
{
#define SET_CONST(name, value)                     \
    push(OBJ_VAL(copyString(name, strlen(name)))); \
    push(NUMBER_VAL(value));                       \
    tableSet(table, vm.stack[0], vm.stack[1]);     \
    pop();                                         \
    pop();

#define SET_CONST_PTR(name, value)                 \
    push(OBJ_VAL(copyString(name, strlen(name)))); \
    push(POINTER_VAL(value));                      \
    tableSet(table, vm.stack[0], vm.stack[1]);     \
    pop();                                         \
    pop();

    SET_CONST_PTR("stdin", stdin);
    SET_CONST_PTR("stdout", stdout);
    SET_CONST_PTR("stderr", stderr);
    SET_CONST("SEEK_END", SEEK_END);
    SET_CONST("SEEK_SET", SEEK_SET);
    SET_CONST("SEEK_CUR", SEEK_CUR);
    SET_CONST("EOF", EOF);
    SET_CONST("FOPEN_MAX", FOPEN_MAX);
    SET_CONST("FILENAME_MAX", FILENAME_MAX);
    SET_CONST("TMP_MAX", TMP_MAX);
#undef SET_CONST_PTR
#undef SET_CONST
}
