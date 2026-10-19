#include "parser.h"
#include "lexer.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Lexer lexer;
    Token current;
    InstructionList *instructions;
    int had_error;
} Parser;

static void advance(Parser *parser)
{
    parser->current = lexer_next_token(&parser->lexer);

    if (parser->current.type == TOKEN_INVALID)
    {
        fprintf(stderr, "Error at line %zu, column %zu: invalid token '%.*s'\n", parser->current.line, parser->current.column, (int)parser->current.length, parser->current.start);
        parser->had_error = 1;
    }
}

static void skip_newlines(Parser *parser)
{
    while (parser->current.type == TOKEN_NEWLINE)
    {
        advance(parser);
    }
}

static int token_is_identifier(const Token *token, const char *name)
{
    size_t name_length = strlen(name);

    return token->type == TOKEN_IDENTIFIER && token->length == name_length && strncmp(token->start, name, name_length) == 0;
}

static int expect_token(Parser *parser, TokenType expected, const char *description)
{
    if (parser->current.type != expected)
    {
        fprintf(stderr, "Error at line %zu, column %zu: expected %s\n", parser->current.line, parser->current.column, description);
        parser->had_error = 1;
        return 0;
    }

    advance(parser);
    return !parser->had_error;
}

static int parse_register_number(Parser *parser, uint8_t *register_number)
{
    Token token = parser->current;

    if (token.type != TOKEN_IDENTIFIER || token.length < 2 || (token.start[0] != 'x' && token.start[0] != 'X'))
    {
        fprintf(stderr, "Error at line %zu, column %zu: expected register x0-x30 or xzr\n", token.line, token.column);
        parser->had_error = 1;
        return 0;
    }

    if (token.length == 3 && (token.start[1] == 'z' || token.start[1] == 'Z') && (token.start[2] == 'r' || token.start[2] == 'R'))
    {
        *register_number = 31;
        advance(parser);
        return !parser->had_error;
    }

    if (token.length < 2 || token.length > 3)
    {
        fprintf(stderr, "Error at line %zu, column %zu: invalid register '%.*s'\n", token.line, token.column, (int)token.length, token.start);
        parser->had_error = 1;
        return 0;
    }

    unsigned int number = 0;

    for (size_t i = 1; i < token.length; i++)
    {
        if (!isdigit((unsigned char)token.start[i]))
        {
            fprintf(stderr, "Error at line %zu, column %zu: invalid register '%.*s'\n", token.line, token.column, (int)token.length, token.start);
            parser->had_error = 1;
            return 0;
        }

        number = number * 10U + (unsigned int)(token.start[i] - '0');
    }

    if (number > 30)
    {
        fprintf(stderr, "Error at line %zu, column %zu: register x%u is out of range; use x0-x30 or xzr\n", token.line, token.column, number);
        parser->had_error = 1;
        return 0;
    }

    *register_number = (uint8_t)number;
    advance(parser);
    return !parser->had_error;
}

static int parse_immediate_value(Parser *parser, uint64_t *value)
{
    Token token = parser->current;

    if (token.type != TOKEN_NUMBER)
    {
        fprintf(stderr, "Error at line %zu, column %zu: expected an immediate value\n", token.line, token.column);
        parser->had_error = 1;
        return 0;
    }

    char *end = NULL;
    unsigned long long parsed_value;

    if (token.length > 2 && token.start[0] == '0' && (token.start[1] == 'x' || token.start[1] == 'X'))
    {
        parsed_value = strtoull(token.start, &end, 16);
    } else {
        parsed_value = strtoull(token.start, &end, 10);
    }

    if (end != token.start + token.length)
    {
        fprintf(stderr, "Error at line %zu, column %zu: invalid immediate value '%.*s'\n", token.line, token.column, (int)token.length, token.start);
        parser->had_error = 1;
        return 0;
    }

    *value = (uint64_t)parsed_value;
    advance(parser);
    return !parser->had_error;
}

static int expect_comma(Parser *parser)
{
    return expect_token(parser, TOKEN_COMMA, "','");
}

static int parse_mov(Parser *parser)
{
    Instruction instruction = {0};

    instruction.type = INSTRUCTION_MOV_IMMEDIATE;
    instruction.line = parser->current.line;

    if (!parse_register_number(parser, &instruction.rd))
    {
        return 0;
    }

    if (!expect_comma(parser))
    {
        return 0;
    }

    if (!expect_token(parser, TOKEN_HASH, "'#'"))
    {
        return 0;
    }

    if (!parse_immediate_value(parser, &instruction.immediate))
    {
        return 0;
    }

    if (instruction.immediate > 65535)
    {
        fprintf(stderr, "Error at line %zu: immediate value is out of range for the currently supported MOV form\n", instruction.line);
        parser->had_error = 1;
        return 0;
    }

    if (parser->current.type != TOKEN_NEWLINE && parser->current.type != TOKEN_EOF)
    {
        fprintf(stderr, "Error at line %zu, column %zu: unexpected token after MOV operands\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
        return 0;
    }

    if (!instruction_list_append(parser->instructions, &instruction))
    {
        fprintf(stderr, "Error: unable to store instruction\n");
        parser->had_error = 1;
        return 0;
    }

    return 1;
}

static int parse_add(Parser *parser)
{
    Instruction instruction = {0};

    instruction.type = INSTRUCTION_ADD_REGISTER;
    instruction.line = parser->current.line;

    if (!parse_register_number(parser, &instruction.rd))
    {
        return 0;
    }

    if (!expect_comma(parser))
    {
        return 0;
    }

    if (!parse_register_number(parser, &instruction.rn))
    {
        return 0;
    }

    if (!expect_comma(parser))
    {
        return 0;
    }

    if (!parse_register_number(parser, &instruction.rm))
    {
        return 0;
    }

    if (parser->current.type != TOKEN_NEWLINE && parser->current.type != TOKEN_EOF)
    {
        fprintf(stderr, "Error at line %zu, column %zu: unexpected token after ADD operands\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
        return 0;
    }

    if (!instruction_list_append(parser->instructions, &instruction))
    {
        fprintf(stderr, "Error: unable to store instruction\n");
        parser->had_error = 1;
        return 0;
    }

    return 1;
}

static int parse_branch(Parser *parser)
{
    Instruction instruction = {0};

    instruction.type = INSTRUCTION_BRANCH;
    instruction.line = parser->current.line;

    if (parser->current.type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Error at line %zu, column %zu: expected branch target label\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
        return 0;
    }

    Token target = parser->current;

    instruction.target_symbol = malloc(target.length + 1);
    if (instruction.target_symbol == NULL)
    {
        fprintf(stderr, "Error: unable to allocate branch target\n");
        parser->had_error = 1;
        return 0;
    }

    memcpy(instruction.target_symbol, target.start, target.length);
    instruction.target_symbol[target.length] = '\0';

    advance(parser);

    if (parser->current.type != TOKEN_NEWLINE && parser->current.type != TOKEN_EOF)
    {
        fprintf(stderr, "Error at line %zu, column %zu: unexpected token after B operand\n", parser->current.line, parser->current.column);
        free(instruction.target_symbol);
        parser->had_error = 1;
        return 0;
    }

    if (!instruction_list_append(parser->instructions, &instruction))
    {
        fprintf(stderr, "Error: unable to store branch instruction\n");
        free(instruction.target_symbol);
        parser->had_error = 1;
        return 0;
    }

    return 1;
}

static int parse_global_directive(Parser *parser)
{
    advance(parser);

    if (!token_is_identifier(&parser->current, "global"))
    {
        fprintf(stderr, "Error at line %zu, column %zu: expected '.global' directive\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
        return 0;
    }

    advance(parser);

    if (parser->current.type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Error at line %zu, column %zu: expected symbol name after '.global'\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
        return 0;
    }

    Token symbol = parser->current;

    if (!instruction_list_declare_global(parser->instructions, symbol.start, symbol.length))
    {
        fprintf(stderr, "Error at line %zu: unable to declare global symbol\n", symbol.line);
        parser->had_error = 1;
        return 0;
    }

    advance(parser);

    if (parser->current.type != TOKEN_NEWLINE && parser->current.type != TOKEN_EOF)
    {
        fprintf(stderr, "Error at line %zu, column %zu: unexpected token after '.global' symbol\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
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
        fprintf(stderr, "Error at line %zu, column %zu: expected instruction, label, or directive\n", parser->current.line, parser->current.column);
        parser->had_error = 1;
        return 0;
    }

    Token mnemonic = parser->current;
    advance(parser);

    if (parser->current.type == TOKEN_COLON)
    {
        if (!instruction_list_define_label(parser->instructions, mnemonic.start, mnemonic.length))
        {
            fprintf(stderr, "Error at line %zu: duplicate label '%.*s' or unable to store label\n", mnemonic.line, (int)mnemonic.length, mnemonic.start);
            parser->had_error = 1;
            return 0;
        }

        advance(parser);
        return !parser->had_error;
    }

    if (token_is_identifier(&mnemonic, "mov"))
    {
        return parse_mov(parser);
    }

    if (token_is_identifier(&mnemonic, "add"))
    {
        return parse_add(parser);
    }

    if (token_is_identifier(&mnemonic, "b"))
    {
        return parse_branch(parser);
    }

    fprintf(stderr, "Error at line %zu, column %zu: unsupported instruction '%.*s'\n", mnemonic.line, mnemonic.column, (int)mnemonic.length, mnemonic.start);

    parser->had_error = 1;
    return 0;
}

int parser_process(const char *source, InstructionList *instructions)
{
    if (source == NULL || instructions == NULL)
    {
        fprintf(stderr, "Error: invalid parser arguments\n");
        return 1;
    }

    Parser parser = {0};

    parser.instructions = instructions;
    lexer_init(&parser.lexer, source);
    advance(&parser);

    while (parser.current.type != TOKEN_EOF && !parser.had_error)
    {
        skip_newlines(&parser);

        if (parser.current.type == TOKEN_EOF || parser.had_error)
        {
            break;
        }

        if (!parse_statement(&parser))
        {
            break;
        }

        if (parser.current.type == TOKEN_NEWLINE)
        {
            skip_newlines(&parser);
        } else if (parser.current.type != TOKEN_EOF) {
            fprintf(stderr, "Error at line %zu, column %zu: expected end of line\n", parser.current.line, parser.current.column);
            parser.had_error = 1;
        }
    }

    if (parser.had_error)
    {
        return 1;
    }

    instruction_list_print(instructions);
    return 0;
}