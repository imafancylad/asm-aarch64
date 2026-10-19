#ifndef ASM_AARCH64_ENCODER_H
#define ASM_AARCH64_ENCODER_H

#include "instruction.h"

#include <stdint.h>

int encoder_encode_instruction(const Instruction *instruction, uint32_t *machine_code);

int encoder_encode_branch(
    const Instruction *instruction,
    size_t current_index,
    size_t target_index,
    uint32_t *machine_code
);

int encoder_encode_instructions(
    const InstructionList *instructions,
    uint32_t *machine_codes
);

#endif