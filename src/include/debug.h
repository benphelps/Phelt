#ifndef phelt_debug_h
#define phelt_debug_h

#include "chunk.h"
#include "common.h"

void disassembleChunk(Chunk* chunk, const char* name, bool flow);
int  disassembleInstruction(Chunk* chunk, int offset, bool flow);

#endif
