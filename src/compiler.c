#include "compiler.h"

#include "common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
    Token current;
    Token previous;
    bool  hadError;
    bool  panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int   depth;
    bool  isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool    isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_ANONYMOUS,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef struct JumpNode {
    struct JumpNode* next;
    int              jumpPatch;
} JumpNode;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction*     function;
    FunctionType     type;

    Local     locals[UINT8_COUNT];
    int       localCount;
    Upvalue   upvalues[UINT8_COUNT];
    int       scopeDepth;
    bool      isInLoop;
    JumpNode* breakNodes;
    int       loopStart;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool                  hasSuperclass;
} ClassCompiler;

Parser         parser;
Chunk*         compilingChunk;
Compiler*      current        = NULL;
ClassCompiler* currentClass   = NULL;
int            anonymousCount = 0;

static Chunk* currentChunk()
{
    return &current->function->chunk;
}

static void errorAt(Token* token, const char* message)
{
    if (parser.panicMode)
        return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void errorAtCurrent(const char* message)
{
    errorAt(&parser.current, message);
}

static void error(const char* message)
{
    errorAt(&parser.previous, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
        error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn()
{
    // if previous instruction is OP_RETURN, don't emit another one
    if (currentChunk()->count > 0 && currentChunk()->code[currentChunk()->count - 1] == OP_RETURN)
        return;

    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }

    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset]     = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler* compiler, FunctionType type)
{
    compiler->enclosing  = current;
    compiler->function   = newFunction();
    compiler->type       = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->isInLoop   = false;
    compiler->breakNodes = NULL;
    compiler->loopStart  = 0;

    current = compiler;

    if (type != TYPE_SCRIPT) {
        if (type == TYPE_ANONYMOUS) {
            char buffer[30];
            sprintf(buffer, "<anon fn 0x%p>", (void*)compiler->function);
            current->function->name = copyString(buffer, strlen(buffer));
        } else {
            current->function->name = copyString(parser.previous.start, parser.previous.length);
        }
    }

    Local* local      = &current->locals[current->localCount++];
    local->depth      = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start  = "this";
        local->name.length = 4;
    } else {
        local->name.start  = "";
        local->name.length = 0;
    }
}

static ObjFunction* endCompiler()
{
    emitReturn();
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>", true);
    }
#endif

    current = current->enclosing;

    return function;
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->isInLoop = false;
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

static void       expression();
static void       statement();
static void       declaration();
static ParseRule* getRule(TokenType type);
static void       parsePrecedence(Precedence precedence);
static uint8_t    identifierConstant(Token* name);
static int        resolveLocal(Compiler* compiler, Token* name);
static int        resolveUpvalue(Compiler* compiler, Token* name);
static uint8_t    argumentList();
static void       markInitialized();
static void       function(FunctionType type);

static void binary(bool canAssign)
{
    TokenType  operatorType = parser.previous.type;
    ParseRule* rule         = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_PERCENT:
        emitByte(OP_MODULO);
        break;
    case TOKEN_AMPERSAND:
        emitByte(OP_BITWISE_AND);
        break;
    case TOKEN_PIPE:
        emitByte(OP_BITWISE_OR);
        break;
    case TOKEN_CARET:
        emitByte(OP_BITWISE_XOR);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    case TOKEN_SHIFT_LEFT:
        emitByte(OP_SHIFT_LEFT);
        break;
    case TOKEN_SHIFT_RIGHT:
        emitByte(OP_SHIFT_RIGHT);
        break;
    default:
        return; // Unreachable.
    }
}

static void call(bool canAssign)
{
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void index_(bool canAssign)
{
    expression();

    if (match(TOKEN_DOT_DOT)) {
        expression();
        consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
        emitByte(OP_SLICE);
        return;
    }

    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");

    if (canAssign && match(TOKEN_EQUAL)) {
        uint8_t name = identifierConstant(&parser.previous);
        expression();
        emitByte(OP_SET_INDEX);
    } else {
        emitByte(OP_INDEX);
    }
}

static void dot(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount);
    } else {
        TokenType type = parser.current.type;

        switch (type) {
        case TOKEN_PLUS_EQUAL:
        case TOKEN_MINUS_EQUAL:
        case TOKEN_SLASH_EQUAL:
        case TOKEN_STAR_EQUAL: {
            advance();
            emitByte(OP_DUP);
            emitBytes(OP_GET_PROPERTY, name);
            expression();

            switch (type) {
            case TOKEN_PLUS_EQUAL:
                emitByte(OP_ADD);
                break;
            case TOKEN_MINUS_EQUAL:
                emitByte(OP_SUBTRACT);
                break;
            case TOKEN_SLASH_EQUAL:
                emitByte(OP_DIVIDE);
                break;
            case TOKEN_STAR_EQUAL:
                emitByte(OP_MULTIPLY);
                break;
            default:
                // Unreachable.
                break;
            }

            emitBytes(OP_SET_PROPERTY, name);
            break;
        }

        case TOKEN_PLUS_PLUS:
        case TOKEN_MINUS_MINUS: {
            advance();
            emitByte(OP_DUP);
            emitBytes(OP_GET_PROPERTY, name);
            emitByte(type == TOKEN_PLUS_PLUS ? OP_INCREMENT : OP_DECREMENT);
            emitBytes(OP_SET_PROPERTY, name);
            break;
        }

        default:
            emitBytes(OP_GET_PROPERTY, name);
        }
    }
}

static void literal(bool canAssign)
{
    switch (parser.previous.type) {
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    default:
        return; // Unreachable.
    }
}

static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    if (parser.previous.start[0] == '0' && parser.previous.start[1] == 'x') {
        char* end;
        long  value = strtol(parser.previous.start, &end, 16);
        emitConstant(NUMBER_VAL(value));
        return;
    }

    if (parser.previous.start[0] == '0' && parser.previous.start[1] == 'b') {
        char* end;
        long  value = strtol(parser.previous.start + 2, &end, 2);
        emitConstant(NUMBER_VAL(value));
        return;
    }

    if (parser.previous.start[0] == '0' && parser.previous.start[1] == 'o') {
        char* end;
        long  value = strtol(parser.previous.start + 2, &end, 8);
        emitConstant(NUMBER_VAL(value));
        return;
    }

    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump  = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void and_(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int     arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg   = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_PLUS_EQUAL)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_ADD);
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_MINUS_EQUAL)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_SUBTRACT);
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_STAR_EQUAL)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_MULTIPLY);
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_SLASH_EQUAL)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_DIVIDE);
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_SHIFT_RIGHT_EQUAL)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_SHIFT_RIGHT);
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_SHIFT_LEFT_EQUAL)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_SHIFT_LEFT);
        emitBytes(setOp, (uint8_t)arg);
        return;
    } else if (match(TOKEN_PLUS_PLUS)) {
        namedVariable(name, false);
        emitByte(OP_INCREMENT);
        emitBytes(setOp, (uint8_t)arg);
    } else if (match(TOKEN_MINUS_MINUS)) {
        namedVariable(name, false);
        emitByte(OP_DECREMENT);
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static Token syntheticToken(utf8_int8_t* text)
{
    Token token;
    token.start  = text;
    token.length = (int)strlen(text);
    return token;
}

static void super_(bool canAssign)
{
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

static void this(bool canAssign)
{
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
}

static void unary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        return; // Unreachable.
    }
}

static void table(bool canAssign)
{
    uint8_t numElements = 0;
    if (!check(TOKEN_RIGHT_BRACE)) {
        do {
            numElements++;
            if (match(TOKEN_LEFT_BRACKET)) {
                expression();
                consume(TOKEN_RIGHT_BRACKET, "Expect ']' after table key literal.");
                consume(TOKEN_COLON, "Expect ':' after table key.");
                expression();
            } else if (match(TOKEN_IDENTIFIER)) {
                identifierConstant(&parser.previous);
                emitConstant(OBJ_VAL(copyString(parser.previous.start, parser.previous.length)));
                consume(TOKEN_COLON, "Expect ':' after table key.");
                expression();
            } else {
                expression();
                consume(TOKEN_COLON, "Expect ':' after table key.");
                expression();
            }
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after table elements.");
    emitBytes(OP_SET_TABLE, numElements);
}

static void array(bool canAssign)
{
    uint8_t numElements = 0;
    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            numElements++;
            expression();
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array elements.");
    emitBytes(OP_SET_ARRAY, numElements);
}

static void anonFunDeclaration(bool canAssign)
{
    markInitialized();
    function(TYPE_ANONYMOUS);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]        = { grouping, call, PREC_CALL },
    [TOKEN_RIGHT_PAREN]       = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACE]        = { table, NULL, PREC_NONE },
    [TOKEN_RIGHT_BRACE]       = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACKET]      = { array, index_, PREC_CALL },
    [TOKEN_RIGHT_BRACKET]     = { NULL, NULL, PREC_NONE },
    [TOKEN_COMMA]             = { NULL, NULL, PREC_NONE },
    [TOKEN_DOT]               = { NULL, dot, PREC_CALL },
    [TOKEN_DOT_DOT]           = { NULL, NULL, PREC_NONE },
    [TOKEN_MINUS]             = { unary, binary, PREC_TERM },
    [TOKEN_MINUS_MINUS]       = { NULL, NULL, PREC_NONE },
    [TOKEN_MINUS_EQUAL]       = { NULL, NULL, PREC_NONE },
    [TOKEN_PLUS]              = { NULL, binary, PREC_TERM },
    [TOKEN_PLUS_PLUS]         = { NULL, NULL, PREC_NONE },
    [TOKEN_PLUS_EQUAL]        = { NULL, NULL, PREC_NONE },
    [TOKEN_SEMICOLON]         = { NULL, NULL, PREC_NONE },
    [TOKEN_COLON]             = { NULL, NULL, PREC_NONE },
    [TOKEN_SLASH]             = { NULL, binary, PREC_FACTOR },
    [TOKEN_SLASH_EQUAL]       = { NULL, NULL, PREC_NONE },
    [TOKEN_STAR]              = { NULL, binary, PREC_FACTOR },
    [TOKEN_STAR_EQUAL]        = { NULL, NULL, PREC_NONE },
    [TOKEN_PERCENT]           = { NULL, binary, PREC_FACTOR },
    [TOKEN_AMPERSAND]         = { NULL, binary, PREC_FACTOR },
    [TOKEN_PIPE]              = { NULL, binary, PREC_FACTOR },
    [TOKEN_CARET]             = { NULL, binary, PREC_FACTOR },
    [TOKEN_SHIFT_LEFT]        = { NULL, binary, PREC_FACTOR },
    [TOKEN_SHIFT_LEFT_EQUAL]  = { NULL, NULL, PREC_NONE },
    [TOKEN_SHIFT_RIGHT]       = { NULL, binary, PREC_FACTOR },
    [TOKEN_SHIFT_RIGHT_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_BANG]              = { unary, NULL, PREC_NONE },
    [TOKEN_BANG_EQUAL]        = { NULL, binary, PREC_EQUALITY },
    [TOKEN_EQUAL]             = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL_EQUAL]       = { NULL, binary, PREC_EQUALITY },
    [TOKEN_GREATER]           = { NULL, binary, PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL]     = { NULL, binary, PREC_COMPARISON },
    [TOKEN_LESS]              = { NULL, binary, PREC_COMPARISON },
    [TOKEN_LESS_EQUAL]        = { NULL, binary, PREC_COMPARISON },
    [TOKEN_IDENTIFIER]        = { variable, NULL, PREC_NONE },
    [TOKEN_STRING]            = { string, NULL, PREC_NONE },
    [TOKEN_NUMBER]            = { number, NULL, PREC_NONE },
    [TOKEN_AND]               = { NULL, and_, PREC_AND },
    [TOKEN_CLASS]             = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSE]              = { NULL, NULL, PREC_NONE },
    [TOKEN_FALSE]             = { literal, NULL, PREC_NONE },
    [TOKEN_FOR]               = { NULL, NULL, PREC_NONE },
    [TOKEN_FUN]               = { anonFunDeclaration, NULL, PREC_NONE },
    [TOKEN_IF]                = { NULL, NULL, PREC_NONE },
    [TOKEN_SWITCH]            = { NULL, NULL, PREC_NONE },
    [TOKEN_CASE]              = { NULL, binary, PREC_EQUALITY },
    [TOKEN_DEFAULT]           = { NULL, NULL, PREC_NONE },
    [TOKEN_NIL]               = { literal, NULL, PREC_NONE },
    [TOKEN_OR]                = { NULL, or_, PREC_OR },
    [TOKEN_DUMP]              = { NULL, NULL, PREC_NONE },
    [TOKEN_RETURN]            = { NULL, NULL, PREC_NONE },
    [TOKEN_SUPER]             = { super_, NULL, PREC_NONE },
    [TOKEN_THIS]              = { this, NULL, PREC_NONE },
    [TOKEN_TRUE]              = { literal, NULL, PREC_NONE },
    [TOKEN_LET]               = { NULL, NULL, PREC_NONE },
    [TOKEN_WHILE]             = { NULL, NULL, PREC_NONE },
    [TOKEN_IMPORT]            = { NULL, NULL, PREC_NONE },
    [TOKEN_ERROR]             = { NULL, NULL, PREC_NONE },
    [TOKEN_EOF]               = { NULL, NULL, PREC_NONE },
};

static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t identifierConstant(Token* name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index,
    bool isLocal)
{
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index   = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, Token* name)
{
    if (compiler->enclosing == NULL)
        return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;

        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Token name)
{
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local      = &current->locals[current->localCount++];
    local->name       = name;
    local->depth      = -1;
    local->isCaptured = false;
}

static void declareVariable()
{
    if (current->scopeDepth == 0)
        return;

    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized()
{
    if (current->scopeDepth == 0)
        return;

    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList()
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static ParseRule* getRule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    if (match(TOKEN_LEFT_BRACE)) {
        block();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after function expression.");
        emitByte(OP_RETURN);
    }

    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void classDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;

    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);
    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing     = currentClass;
    currentClass                = &classCompiler;

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);

    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_IDENTIFIER)) {
            uint8_t      constant = identifierConstant(&parser.previous);
            FunctionType type     = TYPE_METHOD;
            if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
                type = TYPE_INITIALIZER;
            }

            function(type);
            emitBytes(OP_METHOD, constant);
        } else if (match(TOKEN_LET)) {
            uint8_t property = parseVariable("Expect property name.");

            if (match(TOKEN_EQUAL)) {
                expression();
            } else {
                emitByte(OP_NIL);
            }

            consume(TOKEN_SEMICOLON, "Expect ';' after property declaration.");
            emitBytes(OP_PROPERTY, property);
        } else {
            error("Expect method or property declarations.");
        }
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void funDeclaration()
{
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration()
{
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }

    if (parser.previous.type != TOKEN_SEMICOLON) {
        consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    }

    defineVariable(global);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void switchStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after value.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

    int state = 0; // 0: before all cases, 1: before default, 2: after default.
    int caseEnds[MAX_CASES];
    int caseCount        = 0;
    int previousCaseSkip = -1;

    while (!match(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
            TokenType caseType = parser.previous.type;

            if (state == 2) {
                error("Can't have another case or default after the default case.");
            }

            if (state == 1) {
                // At the end of the previous case, jump over the others.
                caseEnds[caseCount++] = emitJump(OP_JUMP);

                // Patch its condition to jump to the next case (this one).
                patchJump(previousCaseSkip);
                emitByte(OP_POP);
            }

            if (caseType == TOKEN_CASE) {
                state = 1;

                // See if the case is equal to the value.
                emitByte(OP_DUP);
                expression();

                consume(TOKEN_COLON, "Expect ':' after case value.");

                emitByte(OP_EQUAL);
                previousCaseSkip = emitJump(OP_JUMP_IF_FALSE);

                // Pop the comparison result.
                emitByte(OP_POP);
            } else {
                state = 2;
                consume(TOKEN_COLON, "Expect ':' after default.");
                previousCaseSkip = -1;
            }
        } else {
            // Otherwise, it's a statement inside the current case.
            if (state == 0) {
                error("Can't have statements before any case.");
            }
            statement();
        }
    }

    // If we ended without a default case, patch its condition jump.
    if (state == 1) {
        patchJump(previousCaseSkip);
        emitByte(OP_POP);
    }

    // Patch all the case jumps to the end.
    for (int i = 0; i < caseCount; i++) {
        patchJump(caseEnds[i]);
    }

    emitByte(OP_POP); // The switch value.
}

static void breakStatement()
{
    if (!current->isInLoop)
        error("Break must in a loop.");

    JumpNode* node      = ALLOCATE(JumpNode, 1);
    node->jumpPatch     = emitJump(OP_JUMP);
    node->next          = current->breakNodes;
    current->breakNodes = node;

    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'");
}

static void continueStatement()
{
    if (!current->isInLoop)
        error("Continue must in a loop.");

    emitLoop(current->loopStart);

    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'");
}

static void patchBreak()
{
    while (current->breakNodes != NULL) {
        patchJump(current->breakNodes->jumpPatch);
        current->breakNodes = current->breakNodes->next;
    }
}

static void whileStatement()
{
    int loopStart      = currentChunk()->count;
    current->loopStart = loopStart;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    current->isInLoop = true;
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);

    // patch break jump
    patchBreak();
}

static void forStatement()
{
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (match(TOKEN_LET)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart      = currentChunk()->count;
    current->loopStart = loopStart;
    int exitJump       = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump       = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart          = incrementStart;
        current->loopStart = loopStart;
        patchJump(bodyJump);
    }

    current->isInLoop = true;
    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    // patch break jump
    patchBreak();

    endScope();
}

static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE))
        statement();
    patchJump(elseJump);
}

static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_DUMP);
}

static void importStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_IMPORT);
}

static void returnStatement()
{
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;
        switch (parser.current.type) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_LET:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_DUMP:
        case TOKEN_RETURN:
            return;

        default:; // Do nothing.
        }

        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_LET)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode)
        synchronize();
}

static void statement()
{
    if (match(TOKEN_IMPORT)) {
        importStatement();
    } else if (match(TOKEN_DUMP)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_CONTINUE)) {
        continueStatement();
    } else if (match(TOKEN_BREAK)) {
        breakStatement();
    } else if (match(TOKEN_SWITCH)) {
        switchStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ObjFunction* compile(const char* sourcePath, utf8_int8_t* source)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError  = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    function->source      = sourcePath;
    return parser.hadError ? NULL : function;
}

void markCompilerRoots()
{
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}