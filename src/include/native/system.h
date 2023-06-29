#ifndef PHELT_NATIVE_SYSTEM_H
#define PHELT_NATIVE_SYSTEM_H

#include "native.h"

extern bool system_env(int argCount, Value* args);
extern bool system_exit(int argCount, Value* args);
extern bool system_time(int argCount, Value* args);
extern bool system_mtime(int argCount, Value* args);
extern bool system_clock(int argCount, Value* args);
extern bool system_sleep(int argCount, Value* args);
extern bool system_usleep(int argCount, Value* args);
extern bool system_print(int argCount, Value* args);
extern bool system_println(int argCount, Value* args);
extern bool system_sprint(int argCount, Value* args);
extern bool system_typeof(int argCount, Value* args);
extern bool system_module(int argCount, Value* args);

#endif
