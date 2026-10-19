#ifndef ASM_AARCH64_ELF_WRITER_H
#define ASM_AARCH64_ELF_WRITER_H

#include "instruction.h"

int elf_writer_write_object(const char *path, const InstructionList *instructions);

#endif