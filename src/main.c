#include "source.h"
#include "parser.h"
#include "encoder.h"
#include "output.h"

#include <stdint.h>
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
            "  %s <input.s> [-o <output.bin>]\n"
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
    const char *input_path = NULL;
    const char *output_path = NULL;

    if (argc == 1)
    {
        print_usage(stdout);
        return 0;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            if (argc != 2)
            {
                fprintf(stderr, "%s: --help cannot be combined with other arguments.\n", PROGRAM_NAME);
                return 2;
            }

            print_usage(stdout);
            return 0;
        }

        if (strcmp(argv[i], "--version") == 0)
        {
            if (argc != 2)
            {
                fprintf(stderr, "%s: --version cannot be combined with other arguments.\n", PROGRAM_NAME);
                return 2;
            }

            printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
            return 0;
        }

        if (strcmp(argv[i], "-o") == 0)
        {
            if (output_path != NULL || i + 1 >= argc)
            {
                fprintf(stderr, "%s: -o requires one output path and may be used only once.\n", PROGRAM_NAME);
                return 2;
            }

            output_path = argv[++i];

            if (output_path[0] == '\0' || output_path[0] == '-')
            {
                fprintf(stderr, "%s: invalid output path.\n", PROGRAM_NAME);
                return 2;
            }

            continue;
        }

        if (argv[i][0] == '-')
        {
            fprintf(stderr, "%s: unknown option '%s'.\n" "Run '%s --help' for usage information.\n", PROGRAM_NAME, argv[i], PROGRAM_NAME);
            return 2;
        }

        if (input_path != NULL)
        {
            fprintf(stderr, "%s: multiple input files are not supported.\n", PROGRAM_NAME);
            return 2;
        }

        input_path = argv[i];
    }

    if (input_path == NULL)
    {
        fprintf(stderr, "%s: no input file specified.\n", PROGRAM_NAME);
        print_usage(stderr);
        return 2;
    }

    if (output_path != NULL && strcmp(input_path, output_path) == 0)
    {
        fprintf(stderr, "%s: input and output paths must be different.\n", PROGRAM_NAME);
        return 2;
    }

    char *source = NULL;
    size_t source_length = 0;

    if (source_read_file(input_path, &source, &source_length) != 0)
    {
        return 1;
    }

    (void)source_length;

    InstructionList instructions;
    instruction_list_init(&instructions);

    int result = parser_process(source, &instructions);

    if (result == 0)
    {
        printf("\nMachine code:\n");

        for (size_t i = 0; i < instructions.count; i++)
        {
            uint32_t machine_code;

            if (!encoder_encode_instruction(&instructions.items[i], &machine_code))
            {
                result = 1;
                break;
            }

            printf("%08X\n", (unsigned int)machine_code);
        }
    }

    if (result == 0 && output_path != NULL)
    {
        if (!output_write_binary(output_path, &instructions))
        {
            result = 1;
        } else {
            printf("\nOutput written to: %s\n", output_path);
        }
    }

    instruction_list_destroy(&instructions);
    free(source);

    return result;
}