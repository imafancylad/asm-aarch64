#ifndef ASM_AARCH64_INSTRUCTION_H
#define ASM_AARCH64_INSTRUCTION_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    INSTRUCTION_MOV_IMMEDIATE,
    INSTRUCTION_ADD_REGISTER
} InstructionType;

typedef struct {
    InstructionType type;
    uint8_t rd;
    uint8_t rn;
    uint8_t rm;
    uint64_t immediate;
    size_t line;
} Instruction;

typedef struct {
    Instruction *items;
    size_t count;
    size_t capacity;
} InstructionList;

void instruction_list_init(InstructionList *list);
void instruction_list_destroy(InstructionList *list);
int instruction_list_append(InstructionList *list, const Instruction *instruction);
void instruction_list_print(const InstructionList *list);

#endif