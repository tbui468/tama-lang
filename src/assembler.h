#ifndef TMD_ASSEMBLER_H
#define TMD_ASSEMBLER_H

#include <stdint.h>
#include <stdbool.h>

#include "byte_array.h"
#include "token.h"
#include "ast.h"


struct U32Array {
    uint32_t *elements;
    int count;
    int max_count;
};


struct ALabel {
    struct Token t;
    uint32_t addr;
    bool defined;
    struct U32Array ref_locs;
    struct U32Array rjmp_locs;
};


struct ALabelArray {
    struct ALabel *elements;
    int count;
    int max_count;
};


struct Assembler {
    struct ByteArray buf;
    int program_start_patch;
    int phdr_start_patch;
    int ehdr_size_patch;
    int phdr_size_patch;
    int filesz_patch;
    int memsz_patch;
    int vaddr_patch;
    int paddr_patch;
    uint32_t location;
    uint32_t program_start_loc;
    struct ALabelArray ala;
};


void u32a_init(struct U32Array *ua);
void u32a_free(struct U32Array *ua);
void u32a_add(struct U32Array *ua, uint32_t element);

void al_init(struct ALabel *al, struct Token t, uint32_t addr, bool defined);
void al_free(struct ALabel *al);

void ala_init(struct ALabelArray* ala);
void ala_free(struct ALabelArray *ala);
void ala_add(struct ALabelArray *ala, struct ALabel element);
struct ALabel* ala_get_label(struct ALabelArray *ala, struct Token t);

void assembler_init(struct Assembler *a);
void assembler_free(struct Assembler *a);
void assembler_append_elf_header(struct Assembler *a);
void assembler_append_program_header(struct Assembler *a);
void assemble_node(struct Assembler *a, struct Node *node);
void assembler_append_program(struct Assembler *a, struct NodeArray *na);
void assembler_patch_locations(struct Assembler *a);
void assembler_patch_labels(struct Assembler *a);
void assembler_patch_rjmp(struct Assembler *a);
void assembler_write_binary(struct Assembler *a, char* filename);

#endif //TMD_ASSEMBLER_H
