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
    char *name;
    size_t instruction_index;
    int is_global;
    int is_defined;
} Symbol;

typedef struct {
    Instruction *items;
    size_t count;
    size_t capacity;

    Symbol *symbols;
    size_t symbol_count;
    size_t symbol_capacity;
} InstructionList;

void instruction_list_init(InstructionList *list);
void instruction_list_destroy(InstructionList *list);
int instruction_list_append(InstructionList *list, const Instruction *instruction);
int instruction_list_declare_global(InstructionList *list, const char *name, size_t name_length);
int instruction_list_define_label(InstructionList *list, const char *name, size_t name_length);
void instruction_list_print(const InstructionList *list);

#endif