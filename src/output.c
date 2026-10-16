#include "output.h"
#include "encoder.h"

#include <stdint.h>
#include <stdio.h>

int output_write_binary(const char *path, const InstructionList *instructions)
{
    if (path == NULL || instructions == NULL)
    {
        fprintf(stderr, "Error: invalid output arguments\n");
        return 0;
    }

    FILE *file = fopen(path, "wb");

    if (file == NULL)
    {
        perror("Error opening output file");
        return 0;
    }

    for (size_t i = 0; i < instructions->count; i++)
    {
        uint32_t machine_code;

        if (!encoder_encode_instruction(&instructions->items[i], &machine_code))
        {
            fclose(file);
            return 0;
        }

        unsigned char bytes[4] = {
            (unsigned char)(machine_code & 0xFF),
            (unsigned char)((machine_code >> 8) & 0xFF),
            (unsigned char)((machine_code >> 16) & 0xFF),
            (unsigned char)((machine_code >> 24) & 0xFF)
        };

        if (fwrite(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
        {
            fprintf(stderr, "Error writing output file\n");
            fclose(file);
            return 0;
        }
    }

    if (fclose(file) != 0)
    {
        fprintf(stderr, "Error closing output file\n");
        return 0;
    }

    return 1;
}