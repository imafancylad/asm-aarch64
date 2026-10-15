#include "source.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_NAME "asm-aarch64"
#define PROGRAM_VERSION "0.1.0"

static void print_usage(FILE *stream)
{
    fprintf(stream,
            "%s %s\n"
            "AArch64 assembler.\n"
            "\n"
            "Usage:\n"
            "  %s <input.s>\n"
            "  %s --help\n"
            "  %s --version\n",
            PROGRAM_NAME,
            PROGRAM_VERSION,
            PROGRAM_NAME,
            PROGRAM_NAME,
            PROGRAM_NAME);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        print_usage(stdout);
        return 0;
    }

    if (argc > 2)
    {
        fprintf(stderr,
            "%s: too many arguments.\n"
            "Run '%s --help' for usage information.\n",
            PROGRAM_NAME,
            PROGRAM_NAME);
        return 2;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    {
        print_usage(stdout);
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0)
    {
        printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
        return 0;
    }

    if (argv[1][0] == '-')
    {
        fprintf(stderr,
            "%s: unknown option '%s'.\n"
            "Run '%s --help' for usage information.\n",
            PROGRAM_NAME,
            argv[1],
            PROGRAM_NAME);
        return 2;
    }

    char *source = NULL;
    size_t source_length = 0;

    if (source_read_file(argv[1], &source, &source_length) != 0)
    {
        return 1;
    }

    (void)source_length;

    InstructionList instructions;
    instruction_list_init(&instructions);

    int result = parser_process(source, &instructions);

    instruction_list_destroy(&instructions);
    free(source);
    
    return result;
}