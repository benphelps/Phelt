#ifndef lux_chunk_h
#define lux_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
#define OPCODE(op) OP_##op,
#include "opcodes.h"
#undef OPCODE
} OpCode;

typedef struct
{
    int        count;
    int        capacity;
    uint8_t*   code;
    int*       lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int  addConstant(Chunk* chunk, Value value);
void freeChunk(Chunk* chunk);

#endif