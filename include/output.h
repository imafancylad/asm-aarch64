#ifndef ASM_AARCH64_OUTPUT_H
#define ASM_AARCH64_OUTPUT_H

#include "instruction.h"

int output_write_binary(const char *path, const InstructionList *instructions);

#endif