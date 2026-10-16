#include "encoder.h"

#include <stdio.h>

int encoder_encode_instruction(const Instruction *instruction, uint32_t *machine_code)
{
    if (instruction == NULL || machine_code == NULL)
    {
        fprintf(stderr, "Error: invalid encoder arguments\n");
        return 0;
    }

    switch (instruction->type)
    {
        case INSTRUCTION_MOV_IMMEDIATE:
            /* 
             * MOVZ Xd, #imm16
             * codifica la forma soportada de MOV-immediate
             */
            if (instruction->rd > 31 || instruction->immediate > 65535)
            {
                fprintf(stderr, "Error at line %zu: MOV immediate cannot be encoded\n", instruction->line);
                return 0;
            }

            *machine_code = UINT32_C(0xD2800000) | ((uint32_t)instruction->immediate << 5) | (uint32_t)instruction->rd;
            return 1;

        case INSTRUCTION_ADD_REGISTER:
            /*
             * ADD Xd, Xn, Xm
             * forma registro de 64 bits
             */
            if (instruction->rd > 31 || instruction->rn > 31 || instruction->rm > 31)
            {
                fprintf(stderr, "Error at line %zu: ADD register cannot be encoded\n", instruction->line);
                return 0;
            }

            *machine_code = UINT32_C(0x8B000000) | ((uint32_t)instruction->rm << 16) | ((uint32_t)instruction->rn << 5) | (uint32_t)instruction->rd;
            return 1;

        default:
            fprintf(stderr, "Error at line %zu: unsupported instruction type\n", instruction->line);
            return 0;
    }
}