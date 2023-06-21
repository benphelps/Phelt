#ifndef LUX_NATIVE_FILE_H
#define LUX_NATIVE_FILE_H

#include "native.h"

extern bool _fopen(int argCount, Value* args);
extern bool _fclose(int argCount, Value* args);
extern bool _fwrite(int argCount, Value* args);
extern bool _fread(int argCount, Value* args);

#endif
