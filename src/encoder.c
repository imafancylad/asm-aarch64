#include "encoder.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

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

int encoder_encode_branch(const Instruction *instruction, size_t current_index, size_t target_index, uint32_t *machine_code)
{
    if (instruction == NULL || machine_code == NULL)
    {
        fprintf(stderr, "Error: invalid branch encoder arguments\n");
        return 0;
    }

    int64_t displacement = (int64_t)target_index - (int64_t)current_index;

    if (displacement < -(INT64_C(1) << 25) || displacement >= (INT64_C(1) << 25))
    {
        fprintf(stderr, "Error at line %zu: branch target is out of range\n", instruction->line);
        return 0;
    }

    uint32_t immediate26 = (uint32_t)displacement & UINT32_C(0x03FFFFFF);
    *machine_code = UINT32_C(0x14000000) | immediate26;

    return 1;
}

static int encoder_find_symbol_index(const InstructionList *instructions, const char *name, size_t *target_index)
{
    for (size_t i = 0; i < instructions->symbol_count; i++)
    {
        const Symbol *symbol = &instructions->symbols[i];

        if (symbol->is_defined && strcmp(symbol->name, name) == 0)
        {
            *target_index = symbol->instruction_index;
            return 1;
        }
    }

    return 0;
}

int encoder_encode_instructions(const InstructionList *instructions, uint32_t *machine_codes)
{
    if (instructions == NULL || (instructions->count > 0 && machine_codes == NULL))
    {
        fprintf(stderr, "Error: invalid instruction list encoder arguments\n");
        return 0;
    }

    for (size_t i = 0; i < instructions->count; i++)
    {
        const Instruction *instruction = &instructions->items[i];

        if (instruction->type == INSTRUCTION_BRANCH)
        {
            size_t target_index;

            if (instruction->target_symbol == NULL || !encoder_find_symbol_index(instructions, instruction->target_symbol, &target_index))
            {
                fprintf(stderr, "Error at line %zu: undefined branch target '%s'\n", instruction->line, instruction->target_symbol != NULL ? instruction->target_symbol : "<missing target>");
                return 0;
            }

            if (!encoder_encode_branch(instruction, i, target_index, &machine_codes[i]))
            {
                return 0;
            }
        }
        else if (!encoder_encode_instruction(instruction, &machine_codes[i]))
        {
            return 0;
        }
    }

    return 1;
}
