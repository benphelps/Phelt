#ifndef PHELT_NATIVE_FILE_H
#define PHELT_NATIVE_FILE_H

#include "native.h"

extern bool file_open(int argCount, Value* args);
extern bool file_tmpfile(int argCount, Value* args);
extern bool file_mkstemps(int argCount, Value* args);
extern bool file_close(int argCount, Value* args);
extern bool file_write(int argCount, Value* args);
extern bool file_read(int argCount, Value* args);
extern bool file_seek(int argCount, Value* args);
extern bool file_tell(int argCount, Value* args);
extern bool file_flush(int argCount, Value* args);
extern bool file_getc(int argCount, Value* args);
extern bool file_gets(int argCount, Value* args);
extern bool file_puts(int argCount, Value* args);
extern bool file_putc(int argCount, Value* args);
extern bool file_remove(int argCount, Value* args);
extern bool file_rename(int argCount, Value* args);

extern void fileCallback(Table* table);

#endif
