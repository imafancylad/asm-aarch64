#include "parser.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    Lexer lexer;
    Token current;
    int failed;
} Parser;

static void advance(Parser *parser)
{
    parser->current = lexer_next_token(&parser->lexer);
}

static void report_error(Parser *parser, const char *message)
{
    if (parser->failed)
    {
        return;
    }

    fprintf(stderr, "asm-aarch64: %zu:%zu: %s\n", parser->current.line, parser->current.column, message);
    parser->failed = 1;
}

static int token_is_identifier(const Token *token, const char *name)
{
    size_t length = strlen(name);
    return token->type == TOKEN_IDENTIFIER && token->length == length && memcmp(token->start, name, length) == 0;
}

static int is_register(const Token *token)
{
    size_t i;
    unsigned int number = 0;

    if (token->type != TOKEN_IDENTIFIER || token->length < 2 || token->start[0] != 'x')
    {
        return 0;
    }

    for (i = 1; i < token->length; i++)
    {
        unsigned char c = (unsigned char)token->start[i];

        if (c < '0' || c > '9')
        {
            return 0;
        }

        number = number * 10U + (unsigned int)(c - '0');

        if (number > 30U)
        {
            return 0;
        }
    }

    return 1;
}

static int expect_token(Parser *parser, TokenType type, const char *message)
{
    if (parser->current.type != type)
    {
        report_error(parser, message);
        return 0;
    }

    advance(parser);
    return 1;
}

static int expect_register(Parser *parser)
{
    if (!is_register(&parser->current))
    {
        report_error(parser, "expected a register from x0 to x30.");
        return 0;
    }

    advance(parser);
    return 1;
}

static int parse_global_directive(Parser *parser)
{
    advance(parser);

    if (!token_is_identifier(&parser->current, "global"))
    {
        report_error(parser, "expected '.global' directive.");
        return 0;
    }

    advance(parser);

    if (parser->current.type != TOKEN_IDENTIFIER)
    {
        report_error(parser, "expected a symbol after '.global'.");
        return 0;
    }

    advance(parser);
    return 1;
}

static int parse_mov(Parser *parser)
{
    if (!expect_register(parser))
    {
        return 0;
    }

    if (!expect_token(parser, TOKEN_COMMA, "expected ',' after destination register."))
    {
        return 0;
    }

    if (!expect_token(parser, TOKEN_HASH, "expected '#' before immediate value."))
    {
        return 0;
    }

    if (!expect_token(parser, TOKEN_NUMBER, "expected an immediate number."))
    {
        return 0;
    }

    return 1;
}

static int parse_add(Parser *parser)
{
    if (!expect_register(parser))
    {
        return 0;
    }

    if (!expect_token(parser, TOKEN_COMMA, "expected ',' after destination register."))
    {
        return 0;
    }

    if (!expect_register(parser))
    {
        return 0;
    }

    if (!expect_token(parser, TOKEN_COMMA, "expected ',' after first source register."))
    {
        return 0;
    }

    if (!expect_register(parser))
    {
        return 0;
    }

    return 1;
}

static int parse_statement(Parser *parser)
{
    if (parser->current.type == TOKEN_DOT)
    {
        return parse_global_directive(parser);
    }

    if (parser->current.type != TOKEN_IDENTIFIER)
    {
        report_error(parser, "expected a label, directive or instruction.");
        return 0;
    }

    {
        Token first = parser->current;

        advance(parser);

        if (parser->current.type == TOKEN_COLON)
        {
            advance(parser);
            return 1;
        }

        if (token_is_identifier(&first, "mov"))
        {
            return parse_mov(parser);
        }

        if (token_is_identifier(&first, "add"))
        {
            return parse_add(parser);
        }
    }

    report_error(parser, "unknown instruction or invalid statement.");
    return 0;
}

int parser_process(const char *source)
{
    Parser parser;

    lexer_init(&parser.lexer, source);
    parser.failed = 0;
    advance(&parser);

    while (parser.current.type != TOKEN_EOF && !parser.failed)
    {
        if (parser.current.type == TOKEN_NEWLINE)
        {
            advance(&parser);
            continue;
        }

        if (!parse_statement(&parser))
        {
            break;
        }

        if (parser.current.type != TOKEN_NEWLINE && parser.current.type != TOKEN_EOF)
        {
            report_error(&parser, "unexpected token at end of statement.");
            break;
        }

        if (parser.current.type == TOKEN_NEWLINE)
        {
            advance(&parser);
        }
    }

    if (!parser.failed)
    {
        printf("asm-aarch64: syntax check passed.\n");
        return 0;
    }

    return 1;
}