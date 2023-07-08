#include "chunk.h"
#include "common.h"
#include "memory.h"
#include "vm.h"

void initChunk(Chunk* chunk)
{
    chunk->count    = 0;
    chunk->capacity = 0;
    chunk->code     = NULL;

    initValueArray(&chunk->constants);
    initValueArray(&chunk->lines);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code     = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    writeValueArray(&chunk->lines, NUMBER_VAL(line));
    chunk->count++;
}

void remiteBytes(Chunk* chunk, int index, int amount)
{
    if (index + amount >= chunk->count) {
        return;
    }

    memcpy((chunk->code + index), (chunk->code + index + amount), chunk->count - (index + amount));
    chunk->count -= amount;

    for (int i = 0; i < amount; i++) {
        removeValueArrayAt(&chunk->lines, index);
    }
}

int addConstant(Chunk* chunk, Value value)
{
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
    return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeValueArray(&chunk->lines);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}
