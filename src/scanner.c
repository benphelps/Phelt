#include <stdio.h>
#include <string.h>

#include "common.h"
#include "identifiers.def"
#include "scanner.h"

typedef struct
{
    utf8_int8_t* start;
    utf8_int8_t* current;
    int          line;
} Scanner;

Scanner scanner;

void initScanner(utf8_int8_t* source)
{
    scanner.start   = (utf8_int8_t*)source;
    scanner.current = (utf8_int8_t*)source;
    scanner.line    = 1;
}

static bool isAlpha(utf8_int32_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c > 0x7F;
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isHexDigit(char c)
{
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool isOctalDigit(char c)
{
    return c >= '0' && c <= '7';
}

static bool isBinaryDigit(char c)
{
    return c == '0' || c == '1';
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

utf8_int32_t previous()
{
    return *(scanner.current - 1);
}

utf8_int32_t peek()
{
    utf8_int32_t codepoint = 0;
    utf8codepoint(scanner.current, &codepoint);
    return codepoint;
}

utf8_int32_t peekNext()
{
    if (isAtEnd())
        return '\0';

    utf8_int32_t peek      = 0;
    utf8_int32_t codepoint = 0;
    utf8codepoint(utf8codepoint(scanner.current, &peek), &codepoint);
    return codepoint;
}

utf8_int32_t peekNextNext()
{
    if (isAtEnd())
        return '\0';

    utf8_int32_t peek      = 0;
    utf8_int32_t codepoint = 0;
    utf8codepoint(utf8codepoint(utf8codepoint(scanner.current, &peek), &peek), &codepoint);
    return codepoint;
}

utf8_int32_t advance()
{
    utf8_int32_t codepoint = 0;
    utf8_int8_t* end       = utf8codepoint(scanner.current, &codepoint);
    scanner.current        = end;
    return codepoint;
}

static bool match(utf8_int32_t expected)
{
    if (isAtEnd())
        return false;
    if (peek() != expected)
        return false;
    scanner.current += utf8codepointsize(expected);
    return true;
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type   = type;
    token.start  = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line   = scanner.line;
    return token;
}

static Token errorToken(utf8_int8_t* message)
{
    Token token;
    token.type   = TOKEN_ERROR;
    token.start  = message;
    token.length = (int)strlen(message);
    token.line   = scanner.line;
    return token;
}

static void skipWhitespace()
{
    for (;;) {
        utf8_int32_t c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/') {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd())
                    advance();
            } else if (peekNext() == '*') {
                // A comment goes until the end of the block.
                while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
                    if (peek() == '\n')
                        scanner.line++;
                    advance();
                }
                if (isAtEnd())
                    return;
                advance();
                advance();
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType()
{
    int                      length = scanner.current - scanner.start;
    const struct Identifier* ident  = checkIdentifier(scanner.start, length);
    if (ident != NULL)
        return ident->token;
    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
        advance();
    return makeToken(identifierType());
}

static Token number(const char c)
{
    bool couldBeHex    = c == '0' && peek() == 'x';
    bool couldBeBinary = c == '0' && peek() == 'b';
    bool couldBeOctal  = c == '0' && peek() == 'o';

    if (couldBeHex) {
        advance();
        while (isHexDigit(peek()))
            advance();
    } else if (couldBeBinary) {
        advance();
        while (isBinaryDigit(peek()))
            advance();
    } else if (couldBeOctal) {
        advance();
        while (isOctalDigit(peek()))
            advance();
    } else {
        while (isDigit(peek()))
            advance();

        if ((peek() == '.') && isDigit(peekNext())) {
            // Consume the ".".
            advance();

            while (isDigit(peek()))
                advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string()
{
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STRING);
}

// """heredoc"""
static Token heredoc()
{
    advance();
    advance();

    scanner.start += 2;
    while (!(peek() == '"' && peekNext() == '"' && peekNextNext() == '"') && !isAtEnd()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    advance();
    Token token = makeToken(TOKEN_STRING);

    advance();
    advance();
    return token;
}

static Token stringSingle()
{
    while (peek() != '\'' && !isAtEnd()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STRING);
}

Token scanToken()
{
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    utf8_int32_t c = advance();

    if (isAlpha(c))
        return identifier();

    if (isDigit(c))
        return number(c);

    switch (c) {
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case '[':
        return makeToken(TOKEN_LEFT_BRACKET);
    case ']':
        return makeToken(TOKEN_RIGHT_BRACKET);
    case ';':
        return makeToken(TOKEN_SEMICOLON);
    case ':':
        return makeToken(TOKEN_COLON);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(match('.') ? TOKEN_DOT_DOT : TOKEN_DOT);
    case '-':
        if (match('-'))
            return makeToken(TOKEN_MINUS_MINUS);
        else if (match('='))
            return makeToken(TOKEN_MINUS_EQUAL);
        else
            return makeToken(TOKEN_MINUS);
    case '+':
        if (match('+'))
            return makeToken(TOKEN_PLUS_PLUS);
        else if (match('='))
            return makeToken(TOKEN_PLUS_EQUAL);
        else
            return makeToken(TOKEN_PLUS);
    case '/':
        return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
    case '*':
        return makeToken(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
    case '%':
        return makeToken(TOKEN_PERCENT);
    case '&':
        return makeToken(match('&') ? TOKEN_AND : TOKEN_AMPERSAND);
    case '|':
        return makeToken(match('|') ? TOKEN_OR : TOKEN_PIPE);
    case '^':
        return makeToken(TOKEN_CARET);
    case '!':
        return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        if (match('<'))
            return makeToken(match('=') ? TOKEN_SHIFT_LEFT_EQUAL : TOKEN_SHIFT_LEFT);
        else if (match('='))
            return makeToken(TOKEN_LESS_EQUAL);
        else
            return makeToken(TOKEN_LESS);
    case '>':
        if (match('>'))
            return makeToken(match('=') ? TOKEN_SHIFT_RIGHT_EQUAL : TOKEN_SHIFT_RIGHT);
        else if (match('='))
            return makeToken(TOKEN_GREATER_EQUAL);
        else
            return makeToken(TOKEN_GREATER);
    case '"':
        if (peek() == '"' && peekNext() == '"') {
            return heredoc();
        } else {
            return string();
        }
    case '\'':
        return stringSingle();
    }

    return errorToken("Unexpected character.");
}
