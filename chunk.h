#ifndef lux_chunk_h
#define lux_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    // Constants
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    // Comparison
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    // Binary
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    // Unary
    OP_NOT,
    OP_NEGATE,

    // Expressions
    OP_POP,
    OP_DUP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,

    // Statements
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_RETURN,
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