#ifndef ASM_AARCH64_SOURCE_H
#define ASM_AARCH64_SOURCE_H

#include <stddef.h>

typedef struct {
    const char *text;
    size_t length;
    size_t line_number;
} SourceLine;

typedef int (*SourceLineHandler)(const SourceLine *line, void *context);
int source_process_file(const char *path, SourceLineHandler handler, void *context);
int source_read_file(const char *path, char **buffer, size_t *length);

#endif