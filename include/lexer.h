#ifndef ASM_AARCH64_LEXER_H
#define ASM_AARCH64_LEXER_H

#include <stddef.h>

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_HASH,
    TOKEN_DOT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    size_t length;
    size_t line;
    size_t column;
} Token;

typedef struct {
    const char *source;
    size_t position;
    size_t line;
    size_t column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);
const char *token_type_name(TokenType type);

#endif