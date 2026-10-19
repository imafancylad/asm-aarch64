#include "encoder.h"
#include "instruction.h"

#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    SECTION_NULL = 0,
    SECTION_TEXT,
    SECTION_SYMTAB,
    SECTION_STRTAB,
    SECTION_SHSTRTAB,
    SECTION_COUNT
};

static int write_zeros(FILE *file, size_t count)
{
    static const unsigned char zeros[16] = {0};

    while (count > 0)
    {
        size_t chunk = count < sizeof(zeros) ? count : sizeof(zeros);

        if (fwrite(zeros, 1, chunk, file) != chunk)
        {
            return 0;
        }

        count -= chunk;
    }

    return 1;
}

static int write_padding(FILE *file, size_t *offset, size_t alignment)
{
    size_t padding = (alignment - (*offset % alignment)) % alignment;

    if (!write_zeros(file, padding))
    {
        return 0;
    }

    *offset += padding;
    return 1;
}

static int write_data(FILE *file, size_t *offset, const void *data, size_t size)
{
    if (size > 0 && fwrite(data, 1, size, file) != size)
    {
        return 0;
    }

    *offset += size;
    return 1;
}

static int append_string(char *buffer, size_t capacity, size_t *length, const char *string, uint32_t *string_offset)
{
    size_t string_length = strlen(string) + 1;

    if (*length > capacity || string_length > capacity - *length)
    {
        return 0;
    }

    *string_offset = (uint32_t)*length;
    memcpy(buffer + *length, string, string_length);
    *length += string_length;

    return 1;
}

int output_write_elf(const char *path, const InstructionList *instructions)
{
    if (path == NULL || instructions == NULL)
    {
        fprintf(stderr, "Error: invalid ELF output arguments\n");
        return 0;
    }

    if (instructions->count > UINT32_MAX / sizeof(uint32_t))
    {
        fprintf(stderr, "Error: .text section is too large\n");
        return 0;
    }

    size_t text_size = instructions->count * sizeof(uint32_t);

    size_t symbol_count = 2;

    for (size_t i = 0; i < instructions->symbol_count; i++)
    {
        if (instructions->symbols[i].is_defined)
        {
            symbol_count++;
        }
        else if (instructions->symbols[i].is_global)
        {
            fprintf(stderr, "Error: undefined global symbol '%s'\n", instructions->symbols[i].name);
            return 0;
        }
    }

    if (symbol_count > SIZE_MAX / sizeof(Elf64_Sym))
    {
        fprintf(stderr, "Error: symbol table is too large\n");
        return 0;
    }

    size_t symtab_size = symbol_count * sizeof(Elf64_Sym);
    size_t strtab_capacity = 1;

    for (size_t i = 0; i < instructions->symbol_count; i++)
    {
        if (instructions->symbols[i].is_defined)
        {
            size_t length = strlen(instructions->symbols[i].name) + 1;

            if (length > SIZE_MAX - strtab_capacity)
            {
                fprintf(stderr, "Error: string table is too large\n");
                return 0;
            }

            strtab_capacity += length;
        }
    }

    char *strtab = calloc(strtab_capacity, 1);
    Elf64_Sym *symbols = calloc(symbol_count, sizeof(*symbols));
    uint32_t *symbol_name_offsets = calloc(instructions->symbol_count > 0 ? instructions->symbol_count : 1, sizeof(*symbol_name_offsets));
    unsigned char *text = malloc(text_size > 0 ? text_size : 1);

    if (strtab == NULL || symbols == NULL || symbol_name_offsets == NULL || text == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        free(strtab);
        free(symbols);
        free(symbol_name_offsets);
        free(text);
        return 0;
    }

    size_t strtab_length = 1;

    for (size_t i = 0; i < instructions->count; i++)
    {
        uint32_t machine_code;

        if (!encoder_encode_instruction(&instructions->items[i], &machine_code))
        {
            fprintf(stderr, "Error: unable to encode instruction\n");
            free(strtab);
            free(symbols);
            free(symbol_name_offsets);
            free(text);
            return 0;
        }

        text[i * 4] = (unsigned char)(machine_code & 0xFF);
        text[i * 4 + 1] = (unsigned char)((machine_code >> 8) & 0xFF);
        text[i * 4 + 2] = (unsigned char)((machine_code >> 16) & 0xFF);
        text[i * 4 + 3] = (unsigned char)((machine_code >> 24) & 0xFF);
    }

    size_t symbol_index = 1;

    symbols[symbol_index].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
    symbols[symbol_index].st_shndx = SECTION_TEXT;
    symbol_index++;

    for (size_t i = 0; i < instructions->symbol_count; i++)
    {
        const Symbol *source_symbol = &instructions->symbols[i];

        if (!source_symbol->is_defined)
        {
            continue;
        }

        uint32_t name_offset;

        if (!append_string(strtab, strtab_capacity, &strtab_length, source_symbol->name, &name_offset))
        {
            fprintf(stderr, "Error: unable to build string table\n");
            free(strtab);
            free(symbols);
            free(symbol_name_offsets);
            free(text);
            return 0;
        }

        symbol_name_offsets[i] = name_offset;

        symbols[symbol_index].st_name = name_offset;
        symbols[symbol_index].st_info = ELF64_ST_INFO(source_symbol->is_global ? STB_GLOBAL : STB_LOCAL, STT_NOTYPE);
        symbols[symbol_index].st_shndx = SECTION_TEXT;
        symbols[symbol_index].st_value = source_symbol->instruction_index * sizeof(uint32_t);
        symbols[symbol_index].st_size = 0;
        symbol_index++;
    }

    size_t local_symbol_count = 2;

    for (size_t i = 0; i < instructions->symbol_count; i++)
    {
        if (instructions->symbols[i].is_defined && !instructions->symbols[i].is_global)
        {
            local_symbol_count++;
        }
    }

    /*
     * ELF requires local symbols to precede global symbols.
     * Sort the defined symbols after the null and section entries.
     */
    for (size_t i = 2; i < symbol_count; i++)
    {
        for (size_t j = i + 1; j < symbol_count; j++)
        {
            unsigned char bind_i = ELF64_ST_BIND(symbols[i].st_info);
            unsigned char bind_j = ELF64_ST_BIND(symbols[j].st_info);

            if (bind_i == STB_GLOBAL && bind_j == STB_LOCAL)
            {
                Elf64_Sym temporary = symbols[i];
                symbols[i] = symbols[j];
                symbols[j] = temporary;
            }
        }
    }

    char shstrtab[] = "\0.text\0.symtab\0.strtab\0.shstrtab\0";

    Elf64_Ehdr header = {0};
    memcpy(header.e_ident, ELFMAG, SELFMAG);
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_ident[EI_OSABI] = ELFOSABI_NONE;
    header.e_type = ET_REL;
    header.e_machine = EM_AARCH64;
    header.e_version = EV_CURRENT;
    header.e_ehsize = sizeof(Elf64_Ehdr);
    header.e_shentsize = sizeof(Elf64_Shdr);
    header.e_shnum = SECTION_COUNT;
    header.e_shstrndx = SECTION_SHSTRTAB;

    Elf64_Shdr sections[SECTION_COUNT] = {0};

    sections[SECTION_TEXT].sh_name = 1;
    sections[SECTION_TEXT].sh_type = SHT_PROGBITS;
    sections[SECTION_TEXT].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    sections[SECTION_TEXT].sh_addralign = 4;
    sections[SECTION_TEXT].sh_size = text_size;

    sections[SECTION_SYMTAB].sh_name = 7;
    sections[SECTION_SYMTAB].sh_type = SHT_SYMTAB;
    sections[SECTION_SYMTAB].sh_addralign = 8;
    sections[SECTION_SYMTAB].sh_entsize = sizeof(Elf64_Sym);
    sections[SECTION_SYMTAB].sh_link = SECTION_STRTAB;
    sections[SECTION_SYMTAB].sh_info = local_symbol_count;
    sections[SECTION_SYMTAB].sh_size = symtab_size;

    sections[SECTION_STRTAB].sh_name = 15;
    sections[SECTION_STRTAB].sh_type = SHT_STRTAB;
    sections[SECTION_STRTAB].sh_addralign = 1;
    sections[SECTION_STRTAB].sh_size = strtab_length;

    sections[SECTION_SHSTRTAB].sh_name = 23;
    sections[SECTION_SHSTRTAB].sh_type = SHT_STRTAB;
    sections[SECTION_SHSTRTAB].sh_addralign = 1;
    sections[SECTION_SHSTRTAB].sh_size = sizeof(shstrtab);

    FILE *file = fopen(path, "wb");

    if (file == NULL)
    {
        perror("Error opening ELF output file");
        free(strtab);
        free(symbols);
        free(symbol_name_offsets);
        free(text);
        return 0;
    }

    size_t offset = 0;
    int success = write_data(file, &offset, &header, sizeof(header));

    if (success)
    {
        success = write_padding(file, &offset, 4);
    }

    sections[SECTION_TEXT].sh_offset = offset;

    if (success)
    {
        success = write_data(file, &offset, text, text_size);
    }

    if (success)
    {
        success = write_padding(file, &offset, 8);
    }

    sections[SECTION_SYMTAB].sh_offset = offset;

    if (success)
    {
        success = write_data(file, &offset, symbols, symtab_size);
    }

    sections[SECTION_STRTAB].sh_offset = offset;

    if (success)
    {
        success = write_data(file, &offset, strtab, strtab_length);
    }

    sections[SECTION_SHSTRTAB].sh_offset = offset;

    if (success)
    {
        success = write_data(file, &offset, shstrtab, sizeof(shstrtab));
    }

    if (success)
    {
        success = write_padding(file, &offset, 8);
    }

    header.e_shoff = offset;

    if (success)
    {
        success = write_data(file, &offset, sections, sizeof(sections));
    }

    if (success)
    {
        if (fseek(file, 0, SEEK_SET) != 0 ||
            fwrite(&header, 1, sizeof(header), file) != sizeof(header))
        {
            success = 0;
        }
    }

    if (fclose(file) != 0)
    {
        success = 0;
    }

    free(strtab);
    free(symbols);
    free(symbol_name_offsets);
    free(text);

    if (!success)
    {
        fprintf(stderr, "Error writing ELF object\n");
        return 0;
    }

    return 1;
}