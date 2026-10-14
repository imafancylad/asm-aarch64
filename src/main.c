#include <errno.h>
#include <stdio.h>
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

static int check_input_file(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        fprintf(stderr, "%s: cannot open '%s': %s\n", PROGRAM_NAME, path, strerror(errno));
        return 1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "%s: failed to close '%s': %s\n", PROGRAM_NAME, path, strerror(errno));
        return 1;
    }

    printf("%s: input file '%s' is accessible.\n", PROGRAM_NAME, path);
    fprintf(stderr, "%s: assembly is not implemented yet.\n", PROGRAM_NAME);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        print_usage(stdout);
        return 0;
    }

    if (argc > 2) {
        fprintf(stderr, "%s: too many arguments.\n", PROGRAM_NAME);

        fprintf(stderr, "Run '%s --help' for usage information.\n", PROGRAM_NAME);
        return 2;
    }

    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        print_usage(stdout);
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
        return 0;
    }

    if (argv[1][0] == '-') {
        fprintf(stderr, "%s: unknown option '%s'.\n", PROGRAM_NAME, argv[1]);
        fprintf(stderr, "Run '%s --help' for usage information.\n", PROGRAM_NAME);
        return 2;
    }

    return check_input_file(argv[1]);
}