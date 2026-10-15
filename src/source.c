#define _POSIX_C_SOURCE 200809L

#include "source.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int source_process_file(const char *path, SourceLineHandler handler, void *context)
{
    FILE *file;
    char *buffer = NULL;
    size_t capacity = 0;
    size_t line_number = 0;
    ssize_t length;
    int result = 0;

    if (path == NULL || handler == NULL) {
        fprintf(stderr, "asm-aarch64: invalid source processing arguments.\n");
        return 1;
    }

    file = fopen(path, "r");

    if (file == NULL)
    {
        fprintf(stderr, "asm-aarch64: cannot open '%s': %s\n", path, strerror(errno));
        return 1;
    }

    while ((length = getline(&buffer, &capacity, file)) != -1)
    {
        SourceLine line;

        line_number++;

        line.text = buffer;
        line.length = (size_t)length;
        line.line_number = line_number;

        result = handler(&line, context);

        if (result != 0)
        {
            break;
        }
    }

    if (ferror(file))
    {
        fprintf(stderr, "asm-aarch64: error reading '%s': %s\n", path, strerror(errno));
        result = 1;
    }

    free(buffer);

    if (fclose(file) != 0) 
    {
        fprintf(stderr, "asm-aarch64: error closing '%s': %s\n", path, strerror(errno));
        result = 1;
    }

    return result;
}

int source_read_file(const char *path, char **buffer, size_t *length)
{
    FILE *file;
    char *line_buffer = NULL;
    size_t line_capacity = 0;
    size_t total_length = 0;
    size_t buffer_capacity = 0;
    ssize_t line_length;
    char *result = NULL;
    int status = 0;

    if (path == NULL || buffer == NULL || length == NULL)
    {
        fprintf(stderr, "asm-aarch64: invalid source file arguments.\n");
        return 1;
    }

    *buffer = NULL;
    *length = 0;

    file = fopen(path, "r");

    if (file == NULL)
    {
        fprintf(stderr, "asm-aarch64: cannot open '%s': %s\n", path, strerror(errno));
        return 1;
    }

    while ((line_length = getline(&line_buffer, &line_capacity, file)) != -1)
    {
        size_t chunk_length = (size_t)line_length;
        size_t required;

        if (chunk_length > SIZE_MAX - total_length - 1)
        {
            fprintf(stderr, "asm-aarch64: source file is too large.\n");
            status = 1;
            break;
        }

        required = total_length + chunk_length + 1;

        if (required > buffer_capacity)
        {
            size_t new_capacity = buffer_capacity == 0 ? 256 : buffer_capacity;

            while (new_capacity < required)
            {
                if (new_capacity > SIZE_MAX / 2)
                {
                    new_capacity = required;
                    break;
                }

                new_capacity *= 2;
            }

            char *new_buffer = realloc(result, new_capacity);

            if (new_buffer == NULL)
            {
                fprintf(stderr, "asm-aarch64: memory allocation failed.\n");
                status = 1;
                break;
            }

            result = new_buffer;
            buffer_capacity = new_capacity;
        }

        memcpy(result + total_length, line_buffer, chunk_length);
        total_length += chunk_length;
    }

    if (ferror(file))
    {
        fprintf(stderr, "asm-aarch64: error reading '%s': %s\n", path, strerror(errno));
        status = 1;
    }

    free(line_buffer);

    if (fclose(file) != 0)
    {
        fprintf(stderr, "asm-aarch64: error closing '%s': %s\n", path, strerror(errno));
        status = 1;
    }

    if (status != 0)
    {
        free(result);
        return 1;
    }

    if (result == NULL)
    {
        result = malloc(1);

        if (result == NULL)
        {
            fprintf(stderr, "asm-aarch64: memory allocation failed.\n");
            return 1;
        }
    }

    result[total_length] = '\0';

    *buffer = result;
    *length = total_length;

    return 0;
}