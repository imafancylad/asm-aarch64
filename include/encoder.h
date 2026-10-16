#ifndef ASM_AARCH64_ENCODER_H
#define ASM_AARCH64_ENCODER_H

#include "instruction.h"

#include <stdint.h>

int encoder_encode_instruction(const Instruction *instruction, uint32_t *machine_code);

#endif