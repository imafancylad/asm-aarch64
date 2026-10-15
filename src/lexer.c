#include "lexer.h"
#include <ctype.h>

static int is_identifier_start(unsigned char c)
{
    return isalpha(c) || c == '_' || c == '$';
}

static int is_identifier_char(unsigned char c)
{
    return isalnum(c) || c == '_' || c == '$';
}

static char current_char(const Lexer *lexer)
{
    return lexer->source[lexer->position];
}

static char peek_char(const Lexer *lexer)
{
    if (current_char(lexer) == '\0')
    {
        return '\0';
    }

    return lexer->source[lexer->position + 1];
}

static void advance(Lexer *lexer)
{
    if (current_char(lexer) == '\0')
    {
        return;
    }

    if (current_char(lexer) == '\n')
    {
        lexer->line++;
        lexer->column = 1;

    } else {
        lexer->column++;
    }

    lexer->position++;
}

static Token make_token(const Lexer *lexer, TokenType type, size_t start, size_t line, size_t column)
{
    Token token;

    token.type = type;
    token.start = lexer->source + start;
    token.length = lexer->position - start;
    token.line = line;
    token.column = column;

    return token;
}

void lexer_init(Lexer *lexer, const char *source)
{
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
}

Token lexer_next_token(Lexer *lexer)
{
    size_t start;
    size_t line;
    size_t column;
    unsigned char c;

    for (;;) {
        c = (unsigned char)current_char(lexer);

        if (c == '\0')
        {
            return make_token(lexer, TOKEN_EOF, lexer->position, lexer->line, lexer->column);
        }

        if (c == ' ' || c == '\t' || c == '\r')
        {
            advance(lexer);
            continue;
        }

        if (c == '/' && peek_char(lexer) == '/')
        {
            while (current_char(lexer) != '\0' && current_char(lexer) != '\n')
            {
                advance(lexer);
            }

            continue;
        }

        break;
    }

    start = lexer->position;
    line = lexer->line;
    column = lexer->column;
    c = (unsigned char)current_char(lexer);

    if (c == '\n')
    {
        advance(lexer);
        return make_token(lexer, TOKEN_NEWLINE, start, line, column);
    }

    if (is_identifier_start(c))
    {
        advance(lexer);

        while (is_identifier_char((unsigned char)current_char(lexer)))
        {
            advance(lexer);
        }

        return make_token(lexer, TOKEN_IDENTIFIER, start, line, column);
    }

    if (isdigit(c))
    {
        advance(lexer);

        if (c == '0' && (current_char(lexer) == 'x' || current_char(lexer) == 'X'))
        {
            advance(lexer);

            if (!isxdigit((unsigned char)current_char(lexer))) {
                return make_token(lexer, TOKEN_INVALID, start, line, column);
            }

            while (isxdigit((unsigned char)current_char(lexer)))
            {
                advance(lexer);
            }

        } else {
            while (isdigit((unsigned char)current_char(lexer)))
            {
                advance(lexer);
            }
        }

        return make_token(lexer, TOKEN_NUMBER, start, line, column);
    }

    advance(lexer);

    switch (c) {
        case ',':
            return make_token(lexer, TOKEN_COMMA, start, line, column);

        case ':':
            return make_token(lexer, TOKEN_COLON, start, line, column);

        case '#':
            return make_token(lexer, TOKEN_HASH, start, line, column);

        case '.':
            return make_token(lexer, TOKEN_DOT, start, line, column);

        case '+':
            return make_token(lexer, TOKEN_PLUS, start, line, column);

        case '-':
            return make_token(lexer, TOKEN_MINUS, start, line, column);

        default:
            return make_token(lexer, TOKEN_INVALID, start, line, column);
    }
}

const char *token_type_name(TokenType type)
{
    switch (type) {
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER:     return "NUMBER";
        case TOKEN_COMMA:      return "COMMA";
        case TOKEN_COLON:      return "COLON";
        case TOKEN_HASH:       return "HASH";
        case TOKEN_DOT:        return "DOT";
        case TOKEN_PLUS:       return "PLUS";
        case TOKEN_MINUS:      return "MINUS";
        case TOKEN_NEWLINE:    return "NEWLINE";
        case TOKEN_EOF:        return "EOF";
        case TOKEN_INVALID:    return "INVALID";
    }

    return "UNKNOWN";
}