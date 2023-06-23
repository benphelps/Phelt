#ifndef LUX_NATIVE_FILE_H
#define LUX_NATIVE_FILE_H

#include "native.h"

extern bool _fopen(int argCount, Value* args);
extern bool _tmpfile(int argCount, Value* args);
extern bool _mkstemps(int argCount, Value* args);
extern bool _fclose(int argCount, Value* args);
extern bool _fwrite(int argCount, Value* args);
extern bool _fread(int argCount, Value* args);
extern bool _fseek(int argCount, Value* args);
extern bool _ftell(int argCount, Value* args);
extern bool _fflush(int argCount, Value* args);
extern bool _fgetc(int argCount, Value* args);
extern bool _fgets(int argCount, Value* args);
extern bool _fputs(int argCount, Value* args);
extern bool _fputc(int argCount, Value* args);
extern bool _remove(int argCount, Value* args);
extern bool _rename(int argCount, Value* args);

extern void fileCallback(Table* table);

#endif
