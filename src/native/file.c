#include "native/file.h"
#include "vm.h"
#include <stdio.h>

// FILE * fopen(const char * restrict path, const char * restrict mode);
// let fp = file.open("file.txt", "w+");
bool file_open(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkString(0);
    phelt_checkString(1);

    const char* path = phelt_toCString(0);
    const char* mode = phelt_toCString(1);

    FILE* file = fopen(path, mode);
    if (file == NULL) {
        phelt_error("Failed to open file '%s' with mode '%s'.", path, mode);
        return false;
    }

    phelt_pushPointer(-1, (uintptr_t)file);
    return true;
}

// FILE * tmpfile(void);
// let fp = file.tmpfile()
bool file_tmpfile(int argCount, Value* args)
{
    phelt_checkArgs(0);

    FILE* file = tmpfile();
    if (file == NULL) {
        phelt_error("Failed to create temporary file.");
        return false;
    }

    phelt_pushPointer(-1, (uintptr_t)file);
    return true;
}

// int mkstemp(char *template);
// let fp = file.mkstemps("fileXXXXXX")
bool file_mkstemps(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    char* template = phelt_toCString(0);
    int fd         = mkstemp(template);
    if (fd == -1) {
        phelt_error("Failed to create temporary file.");
        return false;
    }

    FILE* file = fdopen(fd, "w+");
    if (file == NULL) {
        phelt_error("Failed to open temporary file.");
        return false;
    }

    phelt_pushPointer(-1, (uintptr_t)file);
    return true;
}

// int fclose(FILE * stream);
// file.close(fp)
bool file_close(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkPointer(0);

    FILE* file   = (FILE*)phelt_toPointer(0);
    int   result = fclose(file);
    if (result != 0) {
        phelt_error("Failed to close file.");
        return false;
    }

    return true;
}

// size_t fwrite(const void * restrict ptr, size_t size, size_t nitems, FILE * restrict stream);
// bytes_written = file.write(fp, str)
bool file_write(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkPointer(0);
    phelt_checkString(1);

    FILE*      stream = (FILE*)phelt_toPointer(0);
    ObjString* string = phelt_toString(1);

    size_t result = fwrite(string->chars, sizeof(char), string->length, stream);
    if (result != (size_t)string->length) {
        phelt_error("Failed to write to file.");
        return false;
    }

    phelt_pushNumber(-1, result);
    return true;
}

// size_t fread(void * restrict ptr, size_t size, size_t nitems, FILE * restrict stream);
// let str = file.read(fp, bytes)
bool file_read(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkPointer(0);
    phelt_checkNumber(1);

    FILE*  stream = (FILE*)phelt_toPointer(0);
    size_t bytes  = (size_t)phelt_toNumber(1);

    char* buffer = (char*)malloc(bytes);
    if (buffer == NULL) {
        phelt_error("Failed to allocate memory.");
        return false;
    }

    size_t result = fread(buffer, sizeof(char), bytes, stream);
    if (result != bytes) {
        phelt_error("Failed to read from file.");
        return false;
    }

    ObjString* string = copyString(buffer, bytes);
    free(buffer);

    phelt_pushString(-1, string);
    return true;
}

// seek(FILE * stream, long int offset, int whence);
// let pos = file.seek(fp, 0, SEEK_END)
bool file_seek(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkPointer(0);
    phelt_checkNumber(1);
    phelt_checkNumber(2);

    FILE*    stream = (FILE*)phelt_toPointer(0);
    long int offset = (long int)phelt_toNumber(1);
    int      whence = (int)phelt_toNumber(2);

    int result = fseek(stream, offset, whence);
    if (result != 0) {
        phelt_error("Failed to seek file.");
        return false;
    }

    return true;
}

// long int ftell(FILE * stream);
// let pos = file.tell(fp)
bool file_tell(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkPointer(0);

    FILE* stream = (FILE*)phelt_toPointer(0);

    long int result = ftell(stream);
    if (result == -1) {
        phelt_error("Failed to get file position.");
        return false;
    }

    phelt_pushNumber(-1, result);
    return true;
}

// void rewind(FILE * stream);
// file.rewind(fp)
bool file_rewind(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkPointer(0);

    FILE* stream = (FILE*)phelt_toPointer(0);

    rewind(stream);
    return true;
}

// int fflush(FILE * stream);
// file.flush(fp)
bool file_flush(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkPointer(0);

    FILE* stream = (FILE*)phelt_toPointer(0);

    int result = fflush(stream);
    if (result != 0) {
        phelt_error("Failed to flush file.");
        return false;
    }

    return true;
}

// int fgetc(FILE * stream);
// let char = file.getc(fp)
bool file_getc(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkPointer(0);

    FILE* stream = (FILE*)phelt_toPointer(0);

    int result = fgetc(stream);
    if (result == EOF) {
        phelt_error("Failed to get character.");
        return false;
    }

    phelt_pushNumber(-1, result);
    return true;
}

// int fgets(char * str, int num, FILE * stream);
// let str = file.gets(fp, 10)
bool file_gets(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkPointer(0);
    phelt_checkNumber(1);

    FILE* stream = (FILE*)phelt_toPointer(0);
    int   num    = (int)phelt_toNumber(1);
    char  str[num];

    char* result = fgets(str, num, stream);
    if (result == NULL) {
        phelt_error("Failed to get string.");
        return false;
    }

    phelt_pushString(-1, copyString((const char*)str, strlen(str)));
    return true;
}

// int fputc(int character, FILE * stream);
// ile.fputc(fp, 65)
bool file_putc(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkPointer(0);
    phelt_checkNumber(1);

    FILE* stream    = (FILE*)phelt_toPointer(0);
    int   character = (int)phelt_toNumber(1);

    int result = fputc(character, stream);
    if (result == EOF) {
        phelt_error("Failed to put character.");
        return false;
    }

    return true;
}

// int fputs(const char * str, FILE * stream);
// file.puts(fp, "Hello World")
bool file_puts(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkPointer(0);
    phelt_checkString(1);

    FILE*       stream = (FILE*)phelt_toPointer(0);
    const char* str    = phelt_toCString(1);

    int result = fputs(str, stream);
    if (result == EOF) {
        phelt_error("Failed to put string.");
        return false;
    }

    return true;
}

// int remove(const char * filename);
// file.remove("test.txt")
bool file_remove(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    const char* filename = phelt_toCString(0);

    int result = remove(filename);
    if (result != 0) {
        phelt_error("Failed to remove file.");
        return false;
    }

    return true;
}

// int rename(const char * oldname, const char * newname);
// file.rename("test.txt", "test2.txt")
bool file_rename(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkString(0);
    phelt_checkString(1);

    const char* oldname = phelt_toCString(0);
    const char* newname = phelt_toCString(1);

    int result = rename(oldname, newname);
    if (result != 0) {
        phelt_error("Failed to rename file.");
        return false;
    }

    return true;
}

void fileCallback(Table* table)
{
#define SET_CONST(name, value)                     \
    push(NUMBER_VAL(value));                       \
    push(OBJ_VAL(copyString(name, strlen(name)))); \
    tableSet(table, pop(), pop());

#define SET_CONST_PTR(name, value)                 \
    push(POINTER_VAL(value));                      \
    push(OBJ_VAL(copyString(name, strlen(name)))); \
    tableSet(table, pop(), pop());

    SET_CONST_PTR("stdin", (uintptr_t)stdin);
    SET_CONST_PTR("stdout", (uintptr_t)stdout);
    SET_CONST_PTR("stderr", (uintptr_t)stderr);
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
