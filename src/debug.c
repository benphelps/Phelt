#include "debug.h"
#include "object.h"

int in_loop = 0;
int loop_ends[254];
int loop_starts[254];

int in_false_jump = 0;
int false_jumps[254];

int in_jump = 0;
int jumps[254];

int loop_depth = 0;

int moveForward(Chunk* chunk, int offset);

void disassembleChunk(Chunk* chunk, const char* name, bool flow)
{
    in_loop       = 0;
    in_false_jump = 0;
    in_jump       = 0;
    loop_depth    = 0;
    memset(loop_ends, 0, sizeof(loop_ends));
    memset(loop_starts, 0, sizeof(loop_starts));
    memset(false_jumps, 0, sizeof(false_jumps));
    memset(jumps, 0, sizeof(jumps));

    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        uint8_t instruction = chunk->code[offset];

        if (instruction == OP_LOOP) {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            loop_starts[in_loop] = offset + 3 + -1 * jump;
            loop_ends[in_loop]   = offset;
            in_loop++;
        }

        offset = moveForward(chunk, offset);
    }

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset, flow);
    }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset)
{
    uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
    constant |= chunk->code[offset + 2];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int constantInstructionCompound(const char* name, Chunk* chunk, int offset, int length)
{
    printf("%-16s ", name);

    for (int i = 0; i < length; i++) {
        uint16_t constant = (uint16_t)(chunk->code[offset + 1 + (i * 2)] << 8);
        constant |= chunk->code[offset + 2 + (i * 2)];
        printf("%4d '", constant);
        printValue(chunk->constants.values[constant]);
        printf("'");
    }

    printf("\n");

    return offset + 1 + (length * 2);
}

static int shortInstructionCompound(const char* name, Chunk* chunk, int offset, int length)
{
    printf("%-16s ", name);

    for (int i = 0; i < length; i++) {
        uint16_t slot = (uint16_t)(chunk->code[offset + 1 + (i * 2)] << 8);
        slot |= chunk->code[offset + 2 + (i * 2)];
        printf("%4d ", slot);
    }

    printf("\n");

    return offset + 1 + (length * 2);
}

static int invokeInstruction(const char* name, Chunk* chunk, int offset)
{
    uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
    constant |= chunk->code[offset + 2];
    uint16_t argCount = (uint16_t)(chunk->code[offset + 3] << 8);
    argCount |= chunk->code[offset + 4];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 5;
}

static int simpleInstruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int shortInstruction(const char* name, Chunk* chunk, int offset)
{
    uint16_t slot = (uint16_t)(chunk->code[offset + 1] << 8);
    slot |= chunk->code[offset + 2];
    printf("%-16s %4d\n", name, slot);
    return offset + 3;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassembleInstruction(Chunk* chunk, int offset, bool flow)
{
    OpCode instruction   = chunk->code[offset];
    bool   is_false_jump = false;
    bool   is_op_jump    = false;

    if (flow) {
        if (instruction == OP_JUMP_IF_FALSE) {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            false_jumps[in_false_jump] = offset + 3 + 1 * jump;
            in_false_jump++;
            is_false_jump = true;
        }

        if (instruction == OP_JUMP) {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            jumps[in_jump] = offset + 3 + 1 * jump;
            in_jump++;
            is_op_jump = true;
        }

        if (is_false_jump) {
            if (in_false_jump > 1) {
                if (in_false_jump == 1) {
                    printf("├╼");
                } else {
                    if (in_false_jump > 2)
                        printf("┣╼");
                    else
                        printf("┟╼");
                }
            } else {
                printf("┌╼");
            }
        } else {
            bool closed_jump = false;
            int  jump_checks = in_false_jump;
            for (int i = 0; i < jump_checks; i++) {
                if (false_jumps[i] == offset) {
                    if (closed_jump) {
                        printf("\b\b");
                    }

                    if (i == in_false_jump - 1) {
                        if (in_false_jump == 1)
                            printf("└─");
                        else {
                            if (in_false_jump > 2)
                                printf("┣─");
                            else
                                printf("┡─");
                        }
                    } else {
                        if (in_false_jump - 1 > 1)
                            printf("┠─");
                        else if (in_false_jump - 1 == 1)
                            printf("┞─");
                        else
                            printf("┖─");
                    }

                    in_false_jump--;
                    closed_jump = true;
                }
            }

            if (in_false_jump >= 1 && !closed_jump) {
                if (in_false_jump >= 2) {
                    printf("┃ ");
                } else {
                    printf("│ ");
                }
            } else if (!closed_jump) {
                printf("  ");
            }
        }
    }

    printf(" %04d ", offset);

    if (flow) {
        if (is_op_jump) {
            if (in_jump > 1) {
                if (in_jump == 1) {
                    printf("╾┤");
                } else if (in_jump > 2) {
                    printf("╾┨");
                } else {
                    printf("╾┪");
                }
            } else {
                printf("╾┐");
            }
        } else {
            bool closed_jump = false;
            int  jump_checks = in_jump;
            for (int i = 0; i < jump_checks; i++) {
                if (jumps[i] == offset) {
                    if (closed_jump) {
                        printf("\b\b");
                    }

                    if (i == in_jump - 1) {
                        if (in_jump == 1)
                            printf("─┘");
                        else {
                            if (in_jump > 2)
                                printf("─┫");
                            else
                                printf("─┩");
                        }
                    } else {
                        if (in_jump - 1 > 1)
                            printf("─┫");
                        else if (in_jump - 1 == 1)
                            printf("─┪");
                        else
                            printf("─┚");
                    }

                    in_jump--;
                    closed_jump = true;
                }
            }

            if (in_jump >= 1 && !closed_jump) {
                if (in_jump >= 2) {
                    printf(" ┃");
                } else {
                    printf(" │");
                }
            } else if (!closed_jump) {
                printf("  ");
            }
        }

        // loops
        bool loop_edge = false;
        // loop through start and ends to check if we're in a loop
        for (int i = 0; i < in_loop; i++) {
            if (loop_starts[i] == offset) {
                loop_depth++;
                if (loop_depth > 1) {
                    printf("├─");
                } else {
                    printf("┌─");
                }
                loop_edge = true;
            }
        }

        for (int i = 0; i < in_loop; i++) {
            if (loop_ends[i] == offset) {
                if (loop_depth > 1) {
                    printf("├╼");
                } else {
                    printf("└╼");
                }
                loop_depth--;
                loop_edge = true;
            }
        }

        if (!loop_edge) {
            if (loop_depth > 0) {
                printf("│ ");
            } else {
                printf("  ");
            }
        }
    }

    int line = AS_NUMBER(chunk->lines.values[offset]);
    int prev = AS_NUMBER(chunk->lines.values[offset - 1]);

    if (offset > 0 && line == prev) {
        printf("   | ");
    } else {
        printf("%4d ", line);
    }

    switch (instruction) {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:
        return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
        return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
        return simpleInstruction("OP_FALSE", offset);
    case OP_EQUAL:
        return simpleInstruction("OP_EQUAL", offset);
    case OP_NOT_EQUAL:
        return simpleInstruction("OP_NOT_EQUAL", offset);
    case OP_GREATER_EQUAL:
        return simpleInstruction("OP_GREATER_EQUAL", offset);
    case OP_GREATER:
        return simpleInstruction("OP_GREATER", offset);
    case OP_LESS_EQUAL:
        return simpleInstruction("OP_LESS_EQUAL", offset);
    case OP_LESS:
        return simpleInstruction("OP_LESS", offset);
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_MODULO:
        return simpleInstruction("OP_MODULO", offset);
    case OP_BITWISE_AND:
        return simpleInstruction("OP_BITWISE_AND", offset);
    case OP_BITWISE_OR:
        return simpleInstruction("OP_BITWISE_OR", offset);
    case OP_BITWISE_XOR:
        return simpleInstruction("OP_BITWISE_XOR", offset);
    case OP_SHIFT_LEFT:
        return simpleInstruction("OP_SHIFT_LEFT", offset);
    case OP_SHIFT_RIGHT:
        return simpleInstruction("OP_SHIFT_RIGHT", offset);
    case OP_NOT:
        return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OP_INCREMENT:
        return simpleInstruction("OP_INCREMENT", offset);
    case OP_DECREMENT:
        return simpleInstruction("OP_DECREMENT", offset);
    case OP_POP:
        return simpleInstruction("OP_POP", offset);
    case OP_POP_N:
        return byteInstruction("OP_POP_N", chunk, offset);
    case OP_DUP:
        return simpleInstruction("OP_DUP", offset);
    case OP_GET_LOCAL:
        return shortInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_GET_LOCAL_2:
        return shortInstructionCompound("OP_GET_LOCAL_2", chunk, offset, 2);
    case OP_GET_LOCAL_3:
        return shortInstructionCompound("OP_GET_LOCAL_3", chunk, offset, 3);
    case OP_GET_LOCAL_4:
        return shortInstructionCompound("OP_GET_LOCAL_4", chunk, offset, 4);
    case OP_SET_LOCAL:
        return shortInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_SET_LOCAL_2:
        return shortInstructionCompound("OP_SET_LOCAL_2", chunk, offset, 2);
    case OP_SET_LOCAL_3:
        return shortInstructionCompound("OP_SET_LOCAL_3", chunk, offset, 3);
    case OP_SET_LOCAL_4:
        return shortInstructionCompound("OP_SET_LOCAL_4", chunk, offset, 4);
    case OP_GET_GLOBAL:
        return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL_2:
        return constantInstructionCompound("OP_GET_GLOBAL_2", chunk, offset, 2);
    case OP_GET_GLOBAL_3:
        return constantInstructionCompound("OP_GET_GLOBAL_3", chunk, offset, 3);
    case OP_GET_GLOBAL_4:
        return constantInstructionCompound("OP_GET_GLOBAL_4", chunk, offset, 4);
    case OP_DEFINE_GLOBAL:
        return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
        return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE:
        return shortInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
        return shortInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_GET_PROPERTY:
        return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
        return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OP_GET_SUPER:
        return constantInstruction("OP_GET_SUPER", chunk, offset);
    case OP_SET_TABLE:
        return shortInstruction("OP_SET_TABLE", chunk, offset);
    case OP_SET_ARRAY:
        return shortInstruction("OP_SET_ARRAY", chunk, offset);
    case OP_JUMP:
        return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_FORMAT:
        return shortInstruction("OP_FORMAT", chunk, offset);
    case OP_JUMP_IF_FALSE:
        return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
        return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_DUMP:
        return simpleInstruction("OP_DUMP", offset);
    case OP_CALL:
        return shortInstruction("OP_CALL", chunk, offset);
    case OP_CALL_BLIND:
        return shortInstruction("OP_CALL_BLIND", chunk, offset);
    case OP_INDEX:
        return simpleInstruction("OP_INDEX", offset);
    case OP_SET_INDEX:
        return simpleInstruction("OP_SET_INDEX", offset);
    case OP_INVOKE:
        return invokeInstruction("OP_INVOKE", chunk, offset);
    case OP_SUPER_INVOKE:
        return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
    case OP_CLOSURE: {
        offset++;
        uint8_t constant = chunk->code[offset++];
        printf("%-16s %4d ", "OP_CLOSURE", constant);
        printValue(chunk->constants.values[constant]);
        printf("\n");

        ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
        for (int j = 0; j < function->upvalueCount; j++) {
            int isLocal = chunk->code[offset++];
            int index   = chunk->code[offset++];
            printf("%04d      |                     %s %d\n",
                offset - 2, isLocal ? "local" : "upvalue", index);
        }

        return offset;
    }
    case OP_CLOSE_UPVALUE:
        return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    case OP_REENTER:
        return simpleInstruction("OP_REENTER", offset);
    case OP_CLASS:
        return constantInstruction("OP_CLASS", chunk, offset);
    case OP_INHERIT:
        return simpleInstruction("OP_INHERIT", offset);
    case OP_METHOD:
        return constantInstruction("OP_METHOD", chunk, offset);
    case OP_IMPORT:
        return simpleInstruction("OP_IMPORT", offset);
    case OP_SLICE:
        return simpleInstruction("OP_SLICE", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

int moveForward(Chunk* chunk, int offset)
{
    OpCode instruction = chunk->code[offset];

    switch (instruction) {
    case OP_CONSTANT:
        return offset + 3;
    case OP_NIL:
        return offset + 1;
    case OP_TRUE:
        return offset + 1;
    case OP_FALSE:
        return offset + 1;
    case OP_EQUAL:
        return offset + 1;
    case OP_NOT_EQUAL:
        return offset + 1;
    case OP_GREATER_EQUAL:
        return offset + 1;
    case OP_GREATER:
        return offset + 1;
    case OP_LESS_EQUAL:
        return offset + 1;
    case OP_LESS:
        return offset + 1;
    case OP_ADD:
        return offset + 1;
    case OP_SUBTRACT:
        return offset + 1;
    case OP_MULTIPLY:
        return offset + 1;
    case OP_DIVIDE:
        return offset + 1;
    case OP_MODULO:
        return offset + 1;
    case OP_BITWISE_AND:
        return offset + 1;
    case OP_BITWISE_OR:
        return offset + 1;
    case OP_BITWISE_XOR:
        return offset + 1;
    case OP_SHIFT_LEFT:
        return offset + 1;
    case OP_SHIFT_RIGHT:
        return offset + 1;
    case OP_INCREMENT:
        return offset + 1;
    case OP_DECREMENT:
        return offset + 1;
    case OP_NOT:
        return offset + 1;
    case OP_NEGATE:
        return offset + 1;
    case OP_POP:
        return offset + 1;
    case OP_DUP:
        return offset + 1;
    case OP_GET_LOCAL:
        return offset + 3;
    case OP_GET_LOCAL_2:
        return offset + 5;
    case OP_GET_LOCAL_3:
        return offset + 7;
    case OP_GET_LOCAL_4:
        return offset + 9;
    case OP_SET_LOCAL:
        return offset + 3;
    case OP_SET_LOCAL_2:
        return offset + 5;
    case OP_SET_LOCAL_3:
        return offset + 7;
    case OP_SET_LOCAL_4:
        return offset + 9;
    case OP_GET_GLOBAL:
        return offset + 3;
    case OP_GET_GLOBAL_2:
        return offset + 5;
    case OP_GET_GLOBAL_3:
        return offset + 7;
    case OP_GET_GLOBAL_4:
        return offset + 9;
    case OP_DEFINE_GLOBAL:
        return offset + 3;
    case OP_SET_GLOBAL:
        return offset + 3;
    case OP_GET_UPVALUE:
        return offset + 3;
    case OP_SET_UPVALUE:
        return offset + 3;
    case OP_GET_PROPERTY:
        return offset + 3;
    case OP_SET_PROPERTY:
        return offset + 3;
    case OP_GET_SUPER:
        return offset + 3;
    case OP_SET_TABLE:
        return offset + 3;
    case OP_SET_ARRAY:
        return offset + 3;
    case OP_JUMP:
        return offset + 3;
    case OP_FORMAT:
        return offset + 3;
    case OP_JUMP_IF_FALSE:
        return offset + 3;
    case OP_LOOP:
        return offset + 3;
    case OP_DUMP:
        return offset + 1;
    case OP_CALL:
        return offset + 3;
    case OP_CALL_BLIND:
        return offset + 3;
    case OP_INDEX:
        return offset + 1;
    case OP_SET_INDEX:
        return offset + 1;
    case OP_INVOKE:
        return offset + 5;
    case OP_SUPER_INVOKE:
        return offset + 5;
    case OP_CLOSURE: {
        offset++;
        uint8_t      constant = chunk->code[offset++];
        ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
        for (int j = 0; j < function->upvalueCount; j++) {
            offset++; // isLocal
            offset++; // index
        }
        return offset;
    }
    case OP_CLOSE_UPVALUE:
        return offset + 1;
    case OP_RETURN:
        return offset + 1;
    case OP_REENTER:
        return offset + 1;
    case OP_CLASS:
        return offset + 3;
    case OP_INHERIT:
        return offset + 1;
    case OP_METHOD:
        return offset + 3;
    case OP_IMPORT:
        return offset + 1;
    case OP_SLICE:
        return offset + 1;
    default:
        return offset + 1;
    }
}
