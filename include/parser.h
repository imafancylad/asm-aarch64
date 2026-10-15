#ifndef ASM_AARCH64_PARSER_H
#define ASM_AARCH64_PARSER_H

#include "instruction.h"

int parser_process(const char *source, InstructionList *instructions);
// int parser_process(const char *source);

#endif