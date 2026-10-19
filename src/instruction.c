#include "instruction.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void instruction_list_init(InstructionList *list)
{
    if (list == NULL)
    {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;

    list->symbols = NULL;
    list->symbol_count = 0;
    list->symbol_capacity = 0;
}

void instruction_list_destroy(InstructionList *list)
{
    if (list == NULL)
    {
        return;
    }
    
    free(list->items);

    for (size_t i = 0; i < list->symbol_count; i++)
    {
        free(list->symbols[i].name);
    }

    free(list->symbols);

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;

    list->symbols = NULL;
    list->symbol_count = 0;
    list->symbol_capacity = 0;
}

int instruction_list_append(InstructionList *list, const Instruction *instruction)
{
    if (list == NULL || instruction == NULL)
    {
        return 0;
    }

    if (list->count == list->capacity)
    {
        size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;

        if (new_capacity < list->capacity)
        {
            return 0;
        }

        Instruction *new_items = realloc(list->items, new_capacity * sizeof(*list->items));

        if (new_items == NULL)
        {
            return 0;
        }

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = *instruction;
    list->count++;

    return 1;
}

static char *copy_symbol_name(const char *name, size_t length)
{
    char *copy = malloc(length + 1);

    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, name, length);
    copy[length] = '\0';

    return copy;
}

static size_t find_symbol(const InstructionList *list, const char *name, size_t name_length)
{
    for (size_t i = 0; i < list->symbol_count; i++)
    {
        const Symbol *symbol = &list->symbols[i];

        if (strlen(symbol->name) == name_length && memcmp(symbol->name, name, name_length) == 0)
        {
            return i;
        }
    }

    return SIZE_MAX;
}

static int reserve_symbol_slot(InstructionList *list)
{
    if (list->symbol_count < list->symbol_capacity)
    {
        return 1;
    }

    size_t new_capacity = list->symbol_capacity == 0 ? 8 : list->symbol_capacity * 2;

    if (new_capacity < list->symbol_capacity || new_capacity > SIZE_MAX / sizeof(*list->symbols))
    {
        return 0;
    }

    Symbol *new_symbols = realloc(list->symbols, new_capacity * sizeof(*list->symbols));

    if (new_symbols == NULL)
    {
        return 0;
    }

    list->symbols = new_symbols;
    list->symbol_capacity = new_capacity;

    return 1;
}

int instruction_list_declare_global(InstructionList *list, const char *name, size_t name_length)
{
    if (list == NULL || name == NULL || name_length == 0)
    {
        return 0;
    }

    size_t index = find_symbol(list, name, name_length);

    if (index != SIZE_MAX)
    {
        list->symbols[index].is_global = 1;
        return 1;
    }

    if (!reserve_symbol_slot(list))
    {
        return 0;
    }

    char *copy = copy_symbol_name(name, name_length);

    if (copy == NULL)
    {
        return 0;
    }

    Symbol symbol = {
        .name = copy,
        .instruction_index = 0,
        .is_global = 1,
        .is_defined = 0
    };

    list->symbols[list->symbol_count++] = symbol;
    return 1;
}

int instruction_list_define_label(InstructionList *list, const char *name, size_t name_length)
{
    if (list == NULL || name == NULL || name_length == 0)
    {
        return 0;
    }

    size_t index = find_symbol(list, name, name_length);

    if (index != SIZE_MAX)
    {
        Symbol *symbol = &list->symbols[index];

        if (symbol->is_defined)
        {
            return 0;
        }

        symbol->instruction_index = list->count;
        symbol->is_defined = 1;
        return 1;
    }

    if (!reserve_symbol_slot(list))
    {
        return 0;
    }

    char *copy = copy_symbol_name(name, name_length);

    if (copy == NULL)
    {
        return 0;
    }

    Symbol symbol = {
        .name = copy,
        .instruction_index = list->count,
        .is_global = 0,
        .is_defined = 1
    };

    list->symbols[list->symbol_count++] = symbol;
    return 1;
}

void instruction_list_print(const InstructionList *list)
{
    printf("Instruction IR: %zu instruction(s)\n", list->count);

    for (size_t i = 0; i < list->count; i++)
    {
        const Instruction *instruction = &list->items[i];

        switch (instruction->type)
        {
            case INSTRUCTION_MOV_IMMEDIATE:
                printf("%4zu MOV_IMMEDIATE x%u, #%" PRIu64 "\n", instruction->line, (unsigned int)instruction->rd, instruction->immediate);
                break;

            case INSTRUCTION_ADD_REGISTER:
                printf("%4zu  ADD_REGISTER   x%u, x%u, x%u\n", instruction->line, (unsigned int)instruction->rd, (unsigned int)instruction->rn, (unsigned int)instruction->rm);
                break;
        }
    }

    if (list->symbol_count > 0)
    {
        printf("\nSymbols:\n");

        for (size_t i = 0; i < list->symbol_count; i++)
        {
            const Symbol *symbol = &list->symbols[i];

            if (symbol->is_defined)
            {
                printf("  %-16s %-7s offset=0x%zx\n", symbol->name, symbol->is_global ? "GLOBAL" : "LOCAL", symbol->instruction_index * 4);
            } else {
                printf("  %-16s %-7s UNDEFINED\n", symbol->name, symbol->is_global ? "GLOBAL" : "LOCAL");
            }
        }
    }
}
