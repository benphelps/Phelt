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
    OP_MODULO,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,

    // Unary
    OP_NOT,
    OP_NEGATE,
    OP_INCREMENT,
    OP_DECREMENT,

    // Expressions
    OP_POP,
    OP_DUP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_SET_TABLE,
    OP_SET_ARRAY,

    // Statements
    OP_DUMP,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_INDEX,
    OP_SET_INDEX,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,

    // Types
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_PROPERTY,
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
