#include "instruction.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

void instruction_list_init(InstructionList *list)
{
    if (list == NULL)
    {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void instruction_list_destroy(InstructionList *list)
{
    free(list->items);

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
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
}
