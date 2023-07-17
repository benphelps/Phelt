#include "compiler.h"

#include "common.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"

typedef struct
{
    Token       current;
    Token       previous;
    bool        hadError;
    bool        panicMode;
    const char* source;
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
    uint16_t index;
    bool     isLocal;
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

    Local     locals[UINT16_COUNT];
    int       localCount;
    Upvalue   upvalues[UINT16_COUNT];
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
bool           inParamList    = false;

static Chunk* currentChunk(void)
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

static void advance(void)
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

static void emitShort(uint16_t byte)
{
    emitBytes((uint8_t)((byte >> 8) & 0xff), (uint8_t)(byte & 0xff));
}

static void emitOpShort(uint8_t op, uint16_t bytes)
{
    emitByte(op);
    emitShort(bytes);
}

static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
        error("Loop body too large.");

    emitShort((uint16_t)offset);
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn(void)
{
    // if previous instruction is OP_RETURN, don't emit another one
    if (currentChunk()->count > 0 && currentChunk()->code[currentChunk()->count - 1] == OP_RETURN)
        return;

    if (current->type == TYPE_INITIALIZER) {
        emitOpShort(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }

    emitByte(OP_RETURN);
}

static uint16_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

static void emitConstant(Value value)
{
    emitOpShort(OP_CONSTANT, makeConstant(value));
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

static ObjFunction* endCompiler(void)
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

static void beginScope(void)
{
    current->scopeDepth++;
}

static void endScope(void)
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

static void       expression(void);
static void       statement(void);
static void       declaration(void);
static ParseRule* getRule(TokenType type);
static void       parsePrecedence(Precedence precedence);
static uint16_t   identifierConstant(Token* name);
static int        resolveLocal(Compiler* compiler, Token* name);
static int        resolveUpvalue(Compiler* compiler, Token* name);
static uint16_t   argumentList(void);
static void       markInitialized(void);
static void       function(FunctionType type, int line);

static void binary(bool canAssign)
{
    UNUSED(canAssign);

    TokenType  operatorType = parser.previous.type;
    ParseRule* rule         = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_BANG_EQUAL:
        emitByte(OP_NOT_EQUAL);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitByte(OP_GREATER_EQUAL);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitByte(OP_LESS_EQUAL);
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
    UNUSED(canAssign);
    uint16_t argCount = argumentList();
    emitOpShort(OP_CALL, argCount);
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
        identifierConstant(&parser.previous);
        expression();
        emitByte(OP_SET_INDEX);
    } else {
        emitByte(OP_INDEX);
    }
}

static void dot(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint16_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitOpShort(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        uint16_t argCount = argumentList();
        emitOpShort(OP_INVOKE, name);
        emitShort(argCount);
    } else {
        TokenType type = parser.current.type;

        switch (type) {
        case TOKEN_PLUS_EQUAL:
        case TOKEN_MINUS_EQUAL:
        case TOKEN_SLASH_EQUAL:
        case TOKEN_STAR_EQUAL: {
            advance();
            emitByte(OP_DUP);
            emitOpShort(OP_GET_PROPERTY, name);
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

            emitOpShort(OP_SET_PROPERTY, name);
            break;
        }

        case TOKEN_PLUS_PLUS:
        case TOKEN_MINUS_MINUS: {
            advance();
            emitByte(OP_DUP);
            emitOpShort(OP_GET_PROPERTY, name);
            emitByte(type == TOKEN_PLUS_PLUS ? OP_INCREMENT : OP_DECREMENT);
            emitOpShort(OP_SET_PROPERTY, name);
            break;
        }

        default:
            emitOpShort(OP_GET_PROPERTY, name);
        }
    }
}

static void literal(bool canAssign)
{
    UNUSED(canAssign);
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
    UNUSED(canAssign);

    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    UNUSED(canAssign);

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
    UNUSED(canAssign);

    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump  = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void and_(bool canAssign)
{
    UNUSED(canAssign);

    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void string(bool canAssign)
{
    UNUSED(canAssign);

    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));

    if (match(TOKEN_PERCENT)) {
        if (match(TOKEN_LEFT_PAREN)) {
            uint16_t argCount = argumentList();
            emitOpShort(OP_FORMAT, argCount);
        } else {
            error("Expect '(' after string interpolation.");
        }
    }
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

    arg = (uint16_t)arg;

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_PLUS_EQUAL)) {
        emitOpShort(getOp, arg);
        expression();
        emitByte(OP_ADD);
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_MINUS_EQUAL)) {
        emitOpShort(getOp, arg);
        expression();
        emitByte(OP_SUBTRACT);
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_STAR_EQUAL)) {
        emitOpShort(getOp, arg);
        expression();
        emitByte(OP_MULTIPLY);
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_SLASH_EQUAL)) {
        emitOpShort(getOp, arg);
        expression();
        emitByte(OP_DIVIDE);
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_SHIFT_RIGHT_EQUAL)) {
        emitOpShort(getOp, arg);
        expression();
        emitByte(OP_SHIFT_RIGHT);
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_SHIFT_LEFT_EQUAL)) {
        emitOpShort(getOp, arg);
        expression();
        emitByte(OP_SHIFT_LEFT);
        emitOpShort(setOp, arg);
        return;
    } else if (match(TOKEN_PLUS_PLUS)) {
        namedVariable(name, false);
        emitByte(OP_INCREMENT);
        emitOpShort(setOp, arg);
    } else if (match(TOKEN_MINUS_MINUS)) {
        namedVariable(name, false);
        emitByte(OP_DECREMENT);
        emitOpShort(setOp, arg);
    } else {
        emitOpShort(getOp, arg);
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
    UNUSED(canAssign);

    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint16_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        uint16_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitOpShort(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitOpShort(OP_GET_SUPER, name);
    }
}

static void this(bool canAssign)
{
    UNUSED(canAssign);

    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
}

static void unary(bool canAssign)
{
    UNUSED(canAssign);

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
    UNUSED(canAssign);

    uint16_t numElements = 0;
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
    emitOpShort(OP_SET_TABLE, numElements);
}

static void array(bool canAssign)
{
    UNUSED(canAssign);

    uint16_t numElements = 0;
    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            numElements++;
            expression();
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array elements.");
    emitOpShort(OP_SET_ARRAY, numElements);
}

static void anonFunDeclaration(bool canAssign)
{
    UNUSED(canAssign);

    markInitialized();
    function(TYPE_ANONYMOUS, parser.previous.line);
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

static uint16_t identifierConstant(Token* name)
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

static int addUpvalue(Compiler* compiler, uint16_t index,
    bool isLocal)
{
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT16_COUNT) {
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

        return addUpvalue(compiler, local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, upvalue, false);
    }

    return -1;
}

static void addLocal(Token name)
{
    if (current->localCount == UINT16_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local      = &current->locals[current->localCount++];
    local->name       = name;
    local->depth      = -1;
    local->isCaptured = false;
}

static void declareVariable(void)
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

static uint16_t parseVariable(const char* errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized(void)
{
    if (current->scopeDepth == 0)
        return;

    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint16_t global)
{
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitOpShort(OP_DEFINE_GLOBAL, global);
}

static uint16_t argumentList(void)
{
    inParamList       = true;
    uint16_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == UINT16_MAX) {
                error("Can't have more than 65535 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    inParamList = false;
    return argCount;
}

static ParseRule* getRule(TokenType type)
{
    return &rules[type];
}

static void expression(void)
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block(void)
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type, int line)
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
            uint16_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    if (match(TOKEN_LEFT_BRACE)) {
        block();
    } else {
        expression();
        if (!inParamList) {
            consume(TOKEN_SEMICOLON, "Expect ';' after function expression.");
        }
        emitByte(OP_RETURN);
    }

    ObjFunction* function = endCompiler();
    function->source      = parser.source;
    function->line        = line;
    emitOpShort(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitShort(compiler.upvalues[i].index);
    }
}

static void classDeclaration(void)
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;

    uint16_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitOpShort(OP_CLASS, nameConstant);
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
            uint16_t     constant = identifierConstant(&parser.previous);
            FunctionType type     = TYPE_METHOD;
            if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
                type = TYPE_INITIALIZER;
            }

            function(type, parser.previous.line);
            emitOpShort(OP_METHOD, constant);
        } else if (match(TOKEN_LET)) {
            uint16_t property = parseVariable("Expect property name.");

            if (match(TOKEN_EQUAL)) {
                expression();
            } else {
                emitByte(OP_NIL);
            }

            consume(TOKEN_SEMICOLON, "Expect ';' after property declaration.");
            emitOpShort(OP_PROPERTY, property);
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

static void funDeclaration(void)
{
    uint16_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION, parser.previous.line);
    defineVariable(global);
}

static void varDeclaration(void)
{
    uint16_t global = parseVariable("Expect variable name.");

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

static void expressionStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void switchStatement(void)
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

static void breakStatement(void)
{
    if (!current->isInLoop)
        error("Break must in a loop.");

    JumpNode* node      = ALLOCATE(JumpNode, 1);
    node->jumpPatch     = emitJump(OP_JUMP);
    node->next          = current->breakNodes;
    current->breakNodes = node;

    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'");
}

static void continueStatement(void)
{
    if (!current->isInLoop)
        error("Continue must in a loop.");

    emitLoop(current->loopStart);

    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'");
}

static void patchBreak(void)
{
    while (current->breakNodes != NULL) {
        patchJump(current->breakNodes->jumpPatch);
        current->breakNodes = current->breakNodes->next;
    }
}

static void whileStatement(void)
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

static void forStatement(void)
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

static void ifStatement(void)
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

static void printStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_DUMP);
}

static void importStatement(void)
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_IMPORT);
}

static void returnStatement(void)
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

static void synchronize(void)
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

static void declaration(void)
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

static void statement(void)
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

static void optimizeBinaryConst(Chunk* chunk, int* index)
{
    Value*   constants = chunk->constants.values;
    uint8_t* code      = chunk->code;

    uint16_t arg1 = (code[*index + 1] << 8) | code[*index + 2];
    uint16_t arg2 = (code[*index + 4] << 8) | code[*index + 5];

#define BINARY_OP(op)                                 \
    {                                                 \
        double b        = AS_NUMBER(constants[arg2]); \
        double a        = AS_NUMBER(constants[arg1]); \
        constants[arg1] = NUMBER_VAL(a op b);         \
    }

#define BINARY_OP_INT(op)                                  \
    {                                                      \
        int b           = (int)AS_NUMBER(constants[arg2]); \
        int a           = (int)AS_NUMBER(constants[arg1]); \
        constants[arg1] = NUMBER_VAL(a op b);              \
    }

#define BINARY_OP_BOOL(op)                            \
    {                                                 \
        double b        = AS_NUMBER(constants[arg2]); \
        double a        = AS_NUMBER(constants[arg1]); \
        constants[arg1] = BOOL_VAL(a op b);           \
    }

    switch (chunk->code[*index + 6]) {
    case OP_ADD:
        BINARY_OP(+);
        break;
    case OP_MULTIPLY:
        BINARY_OP(*);
        break;
    case OP_DIVIDE:
        BINARY_OP(/);
        break;
    case OP_SUBTRACT:
        BINARY_OP(-);
        break;
    case OP_MODULO:
        BINARY_OP_INT(%);
        break;
    case OP_BITWISE_AND:
        BINARY_OP_INT(&);
        break;
    case OP_BITWISE_OR:
        BINARY_OP_INT(|);
        break;
    case OP_BITWISE_XOR:
        BINARY_OP_INT(^);
        break;
    case OP_SHIFT_LEFT:
        BINARY_OP_INT(<<);
        break;
    case OP_SHIFT_RIGHT:
        BINARY_OP_INT(>>);
        break;
    case OP_LESS:
        BINARY_OP_BOOL(<);
        break;
    case OP_GREATER:
        BINARY_OP_BOOL(>);
        break;
    default:
        break;
    }

    remiteBytes(chunk, *index + 3, 4);
}

void optimizeChunk(Chunk* chunk)
{
    Value*   constants = chunk->constants.values;
    uint8_t* code      = chunk->code;

    int passes = 0;
    int folds  = 0;

    // calculate the number of jumps
    int jumpCount = 0;
    for (int i = 0; i < chunk->count;) {
        uint8_t instruction = code[i];
        int     movement    = moveForward(chunk, i);
        switch (instruction) {
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_LOOP:
            jumpCount++;
        default:
            i = movement;
            break;
        }
    }

    // array of jump instruction offsets
    int jumps[jumpCount];
    memset(jumps, 0, sizeof(jumps));

    // store jump offsets and targets
    int jumpIndex = 0;
    for (int j = 0; j < chunk->count;) {
        uint8_t instruction = code[j];
        int     movement    = moveForward(chunk, j);
        switch (instruction) {
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_LOOP:
            jumps[jumpIndex++] = j;
        default:
            j = movement;
            break;
        }
    }

    for (int i = 0; i < chunk->count;) {
        uint8_t instruction       = code[i];
        int     movement          = moveForward(chunk, i);
        int     currentAdjustment = 0;

        switch (instruction) {
        case OP_CONSTANT: {
            uint8_t nextInstruction = code[i + 3];
            if (nextInstruction == OP_CONSTANT) {
                uint8_t  operation = code[i + 6];
                uint16_t arg1      = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2      = (code[i + 4] << 8) | code[i + 5];

                switch (operation) {
                case OP_ADD:
                case OP_MULTIPLY:
                case OP_DIVIDE:
                case OP_SUBTRACT:
                case OP_MODULO:
                case OP_BITWISE_AND:
                case OP_BITWISE_OR:
                case OP_BITWISE_XOR:
                case OP_SHIFT_LEFT:
                case OP_SHIFT_RIGHT:
                case OP_LESS:
                case OP_GREATER:
                    if (IS_NUMBER(constants[arg1]) && IS_NUMBER(constants[arg2])) {
                        optimizeBinaryConst(chunk, &i);
                        folds++;
                        currentAdjustment += 4;
                    }
                    break;
                default:
                    i = movement;
                    break;
                }
            } else {
                i = movement;
            }
            break;
        }
        case OP_GET_GLOBAL: {
            uint8_t nextInstruction = code[i + 3];
            if (nextInstruction == OP_GET_GLOBAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 4] << 8) | code[i + 5];

                code[i]     = OP_GET_GLOBAL_2;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                remiteBytes(chunk, i + 5, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_GET_GLOBAL_2: {
            uint8_t nextInstruction = code[i + 5];
            if (nextInstruction == OP_GET_GLOBAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 3] << 8) | code[i + 4];
                uint16_t arg3 = (code[i + 6] << 8) | code[i + 7];

                code[i]     = OP_GET_GLOBAL_3;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                code[i + 5] = arg3 >> 8;
                code[i + 6] = arg3 & 0xff;
                remiteBytes(chunk, i + 7, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_GET_GLOBAL_3: {
            uint8_t nextInstruction = code[i + 7];
            if (nextInstruction == OP_GET_GLOBAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 3] << 8) | code[i + 4];
                uint16_t arg3 = (code[i + 5] << 8) | code[i + 6];
                uint16_t arg4 = (code[i + 8] << 8) | code[i + 9];

                code[i]     = OP_GET_GLOBAL_4;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                code[i + 5] = arg3 >> 8;
                code[i + 6] = arg3 & 0xff;
                code[i + 7] = arg4 >> 8;
                code[i + 8] = arg4 & 0xff;
                remiteBytes(chunk, i + 9, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_GET_LOCAL: {
            uint8_t nextInstruction = code[i + 3];
            if (nextInstruction == OP_GET_LOCAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 4] << 8) | code[i + 5];

                code[i]     = OP_GET_LOCAL_2;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                remiteBytes(chunk, i + 5, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_GET_LOCAL_2: {
            uint8_t nextInstruction = code[i + 5];
            if (nextInstruction == OP_GET_LOCAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 3] << 8) | code[i + 4];
                uint16_t arg3 = (code[i + 6] << 8) | code[i + 7];

                code[i]     = OP_GET_LOCAL_3;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                code[i + 5] = arg3 >> 8;
                code[i + 6] = arg3 & 0xff;
                remiteBytes(chunk, i + 7, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_GET_LOCAL_3: {
            uint8_t nextInstruction = code[i + 7];
            if (nextInstruction == OP_GET_LOCAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 3] << 8) | code[i + 4];
                uint16_t arg3 = (code[i + 5] << 8) | code[i + 6];
                uint16_t arg4 = (code[i + 8] << 8) | code[i + 9];

                code[i]     = OP_GET_LOCAL_4;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                code[i + 5] = arg3 >> 8;
                code[i + 6] = arg3 & 0xff;
                code[i + 7] = arg4 >> 8;
                code[i + 8] = arg4 & 0xff;
                remiteBytes(chunk, i + 9, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }

        case OP_SET_LOCAL: {
            uint8_t nextInstruction = code[i + 3];
            if (nextInstruction == OP_SET_LOCAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 4] << 8) | code[i + 5];

                code[i]     = OP_SET_LOCAL_2;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                remiteBytes(chunk, i + 5, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_SET_LOCAL_2: {
            uint8_t nextInstruction = code[i + 5];
            if (nextInstruction == OP_SET_LOCAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 3] << 8) | code[i + 4];
                uint16_t arg3 = (code[i + 6] << 8) | code[i + 7];

                code[i]     = OP_SET_LOCAL_3;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                code[i + 5] = arg3 >> 8;
                code[i + 6] = arg3 & 0xff;
                remiteBytes(chunk, i + 7, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }
        case OP_SET_LOCAL_3: {
            uint8_t nextInstruction = code[i + 7];
            if (nextInstruction == OP_SET_LOCAL) {
                uint16_t arg1 = (code[i + 1] << 8) | code[i + 2];
                uint16_t arg2 = (code[i + 3] << 8) | code[i + 4];
                uint16_t arg3 = (code[i + 5] << 8) | code[i + 6];
                uint16_t arg4 = (code[i + 8] << 8) | code[i + 9];

                code[i]     = OP_SET_LOCAL_4;
                code[i + 1] = arg1 >> 8;
                code[i + 2] = arg1 & 0xff;
                code[i + 3] = arg2 >> 8;
                code[i + 4] = arg2 & 0xff;
                code[i + 5] = arg3 >> 8;
                code[i + 6] = arg3 & 0xff;
                code[i + 7] = arg4 >> 8;
                code[i + 8] = arg4 & 0xff;
                remiteBytes(chunk, i + 9, 1);
                folds++;
                currentAdjustment += 1;
            } else {
                i = movement;
            }
            break;
        }

        case OP_POP: {
            uint8_t nextInstruction = code[i + 1];
            if (nextInstruction == OP_POP) {
                bool isJumpTarget = false;

                for (int* offset = jumps; offset < jumps + jumpCount; offset++) {
                    uint8_t  instruction       = code[*offset];
                    uint16_t target            = (uint16_t)(code[*offset + 1] << 8) | code[*offset + 2];
                    uint8_t  dir               = instruction == OP_JUMP || instruction == OP_JUMP_IF_FALSE ? 1 : -1;
                    int      targetIndex       = *offset + 3 + dir * (target);
                    uint8_t  targetInstruction = code[targetIndex];
                    if ((i + 1) == targetIndex && (targetInstruction == OP_POP_N || targetInstruction == OP_POP)) {
                        isJumpTarget = true;
                    }
                }

                if (!isJumpTarget) {
                    code[i]     = OP_POP_N;
                    code[i + 1] = 2;
                } else {
                    i = movement;
                }
            } else {
                i = movement;
            }
            break;
        }

        case OP_POP_N: {
            uint8_t nextInstruction = code[i + 2];

            if (nextInstruction == OP_POP) {
                bool isJumpTarget = false;

                for (int* offset = jumps; offset < jumps + jumpCount; offset++) {
                    uint8_t  instruction       = code[*offset];
                    uint16_t target            = (uint16_t)(code[*offset + 1] << 8) | code[*offset + 2];
                    uint8_t  dir               = instruction == OP_JUMP || instruction == OP_JUMP_IF_FALSE ? 1 : -1;
                    int      targetIndex       = *offset + 3 + dir * (target);
                    uint8_t  targetInstruction = code[targetIndex];
                    if ((i + 1) == targetIndex && (targetInstruction == OP_POP_N || targetInstruction == OP_POP)) {
                        isJumpTarget = true;
                    }
                }

                if (!isJumpTarget) {
                    uint8_t count = code[i + 1];
                    code[i + 1]   = count + 1;
                    remiteBytes(chunk, i + 2, 1);
                    currentAdjustment += 1;
                } else {
                    i = movement;
                }
            } else {
                i = movement;
            }

            break;
        }

            // case OP_CALL: {
            //     uint8_t nextInstruction = code[i + 3];
            //     if (nextInstruction == OP_POP) {
            //         code[i] = OP_CALL_BLIND;
            //         remiteBytes(chunk, i + 3, 1);
            //         currentAdjustment += 1;
            //     } else {
            //         i = movement;
            //     }
            // }

        default:
            i = movement;
            break;
        }

        if (currentAdjustment > 0) {
            // fix jump offsets post-fold
            for (int* offset = jumps; offset < jumps + jumpCount; offset++) {
                if (*offset >= i) {
                    *offset = *offset - currentAdjustment;
                }
            }

            // fix jump targets post-fold
            for (int* offset = jumps; offset < jumps + jumpCount; offset++) {
                uint8_t  instruction = code[*offset];
                uint16_t target      = (uint16_t)(code[*offset + 1] << 8) | code[*offset + 2];
                if (instruction == OP_JUMP || instruction == OP_JUMP_IF_FALSE) {
                    int targetIndex = *offset + (target - currentAdjustment) + 3;
                    if (i <= targetIndex && (*offset <= i)) {
                        target -= currentAdjustment;
                        code[*offset + 1] = target >> 8;
                        code[*offset + 2] = target & 0xff;
                    }
                } else if (instruction == OP_LOOP) {
                    int targetIndex = *offset + -(target - currentAdjustment) + 3;
                    if (i >= targetIndex && (i <= *offset)) {
                        target -= currentAdjustment;
                        code[*offset + 1] = target >> 8;
                        code[*offset + 2] = target & 0xff;
                    }
                }
            }
        }

        passes++;
        // printf("\n");
    }

    UNUSED(passes);
    UNUSED(folds);
    // printf("Optimization Passes: %d\n", passes);
    // printf("Constant Folds: %d\n", folds);
}

ObjFunction* compile(const char* sourcePath, utf8_int8_t* source)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError  = false;
    parser.panicMode = false;
    parser.source    = sourcePath;

    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
#ifdef CHUNK_OPTIMIZATION
    optimizeChunk(&function->chunk);
#endif
    // exit(1);
    function->source = sourcePath;
    return parser.hadError ? NULL : function;
}

void markCompilerRoots(void)
{
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}
