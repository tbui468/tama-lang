#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "assembler.h"
#include "memory.h"
#include "error.h"

//second byte
//xx000000
static uint8_t mod_tbl[] = {
    0x00,
    0x40,
    0x80,
    0xc0    //r/m is register
};

enum OpMod {
    MOD_00 = 0,
    MOD_01,
    MOD_10,
    MOD_REG
};

//registers table indices should match with enum TokenType register values
//00xxx000
static uint8_t r_tbl[] = {
    0x00 << 3,   //eax
    0x01 << 3,
    0x02 << 3,
    0x03 << 3,
    0x04 << 3,
    0x05 << 3,
    0x06 << 3,
    0x07 << 3
};

//00000xxx
static uint8_t rm_tbl[] = {
    0x00,   //eax
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07
};

void u32a_init(struct U32Array *ua) {
    ua->elements = NULL;
    ua->count = 0;
    ua->max_count = 0;
}

void u32a_free(struct U32Array *ua) {
    free_arr(ua->elements, sizeof(uint32_t), ua->max_count); 
}

void u32a_add(struct U32Array *ua, uint32_t element) {
    int old_max = ua->max_count;
    if (ua->count + 1 > ua->max_count) {
        ua->max_count *= 2;
        if (ua->max_count == 0)
            ua->max_count = 8;
    }

    ua->elements = alloc_arr(ua->elements, sizeof(uint32_t), old_max, ua->max_count);

    ua->elements[ua->count] = element;
    ua->count++;
}


void al_init(struct ALabel *al, struct Token t, uint32_t addr, bool defined) {
    al->t = t;
    al->addr = addr;
    al->defined = defined;
    u32a_init(&al->ref_locs);
    u32a_init(&al->rjmp_locs);
}

void al_free(struct ALabel *al) {
    u32a_free(&al->ref_locs);
    u32a_free(&al->rjmp_locs);
}


void ala_init(struct ALabelArray* ala) {
    ala->elements = NULL;
    ala->count = 0;
    ala->max_count = 0;
}

void ala_free(struct ALabelArray *ala) {
    for (int i = 0; i < ala->count; i++) {
        al_free(&ala->elements[i]);
    }
    free_arr(ala->elements, sizeof(struct ALabel), ala->max_count); 
}

void ala_add(struct ALabelArray *ala, struct ALabel element) {
    int old_max = ala->max_count;
    if (ala->count + 1 > ala->max_count) {
        ala->max_count *= 2;
        if (ala->max_count == 0)
            ala->max_count = 8;
    }

    ala->elements = alloc_arr(ala->elements, sizeof(struct ALabel), old_max, ala->max_count);

    ala->elements[ala->count] = element;
    ala->count++;
}

struct ALabel* ala_get_label(struct ALabelArray *ala, struct Token t) {
    for (int i = 0; i < ala->count; i++) {
        struct ALabel* cur = &ala->elements[i];
        if (cur->t.len == t.len && strncmp(cur->t.start, t.start, t.len) == 0) {
            return cur;
        }
    }

    return NULL;
}

void assembler_init(struct Assembler *a) {
    ba_init(&a->buf);
    a->program_start_patch = 0;
    a->phdr_start_patch = 0;
    a->ehdr_size_patch = 0;
    a->phdr_size_patch = 0;
    a->filesz_patch = 0;
    a->memsz_patch = 0;
    a->vaddr_patch = 0;
    a->paddr_patch = 0;
    a->location = 0;
    a->program_start_loc = 0;
    ala_init(&a->ala);
}

void assembler_free(struct Assembler *a) {
    ba_free(&a->buf);
    ala_free(&a->ala);
}

void assembler_append_elf_header(struct Assembler *a) {
    uint8_t magic[8];
    magic[0] = 0x7f;
    magic[1] = 'E';
    magic[2] = 'L';
    magic[3] = 'F';
    magic[4] = 1;
    magic[5] = 1;
    magic[6] = 1;
    magic[7] = 0;
    ba_append(&a->buf, magic, 8);

    uint8_t zeros[8] = {0};
    ba_append(&a->buf, zeros, 8);
    uint16_t file_type = 2;
    ba_append_word(&a->buf, file_type);
    uint16_t arch_type = 3;
    ba_append_word(&a->buf, arch_type);
    uint32_t file_version = 1;
    ba_append_double(&a->buf, file_version);

    a->program_start_patch = a->buf.count; //need to add executable offset (0x08048000)
    ba_append(&a->buf, zeros, 4); 
    a->phdr_start_patch = a->buf.count;
    ba_append(&a->buf, zeros, 4);
    ba_append(&a->buf, zeros, 8);
    a->ehdr_size_patch = a->buf.count;
    ba_append(&a->buf, zeros, 2);
    a->phdr_size_patch = a->buf.count;
    ba_append(&a->buf, zeros, 2);

    uint16_t phdr_entries = 1;
    ba_append_word(&a->buf, phdr_entries);
    ba_append(&a->buf, zeros, 6);

    //patch elf header size
    uint16_t ehdr_size = a->buf.count;
    memcpy(&a->buf.bytes[a->ehdr_size_patch], &ehdr_size, 2);
}

void assembler_append_program_header(struct Assembler *a) {
    uint32_t phdr_start = a->buf.count;
    memcpy(&a->buf.bytes[a->phdr_start_patch], &phdr_start, 4);

    uint32_t type = 1; //load
    ba_append_double(&a->buf, type);

    uint32_t offset = 0;
    ba_append_double(&a->buf, offset);

    a->vaddr_patch = a->buf.count;
    uint32_t vaddr = 0;
    ba_append_double(&a->buf, vaddr);

    a->paddr_patch = a->buf.count;
    uint32_t paddr = 0;
    ba_append_double(&a->buf, paddr);

    a->filesz_patch = a->buf.count;
    uint32_t filesz = 0;
    ba_append_double(&a->buf, filesz);

    a->memsz_patch = a->buf.count;
    uint32_t memsz = 0;
    ba_append_double(&a->buf, memsz);

    uint32_t flags = 5;
    ba_append_double(&a->buf, flags);

    uint32_t align = 0x1000;
    ba_append_double(&a->buf, align);

    uint16_t phdr_size = a->buf.count - phdr_start;
    memcpy(&a->buf.bytes[a->phdr_size_patch], &phdr_size, 2);
}


void assemble_node(struct Assembler *a, struct Node *node) {
    switch (node->type) {
        case ANODE_IMM32: {
            struct ANodeImm* imm = (struct ANodeImm*)node;
            ba_append_double(&a->buf, get_double(imm->t));
            break;
        }
        case ANODE_LABEL_DEF: {
            struct ALabel *label = NULL;
            struct ANodeLabelDef* ld = (struct ANodeLabelDef*)node;
            if ((label = ala_get_label(&a->ala, ld->t))) {
                if (label->defined) {
                    ems_add(&ems, ld->t.line, "Assembler Error: Labels cannot be defined more than once.");
                } 
                label->t = ld->t;
                label->addr = a->buf.count;
                label->defined = true;
            } else {
                struct ALabel al;
                al_init(&al, ld->t, a->buf.count, true);
                ala_add(&a->ala, al);
            }
            break;
        }
        case ANODE_LABEL_REF: {
            struct ALabel *label = NULL;
            struct ANodeLabelRef* lr = (struct ANodeLabelRef*)node;
            if (!(label = ala_get_label(&a->ala, lr->t))) {
                struct ALabel al;
                al_init(&al, lr->t, 0, false);
                ala_add(&a->ala, al);
                label = ala_get_label(&a->ala, lr->t);
            }
            u32a_add(&label->ref_locs, a->buf.count);
            ba_append_double(&a->buf, 0x0);
            break;
        }
        case ANODE_OP: {
            struct ANodeOp* o = (struct ANodeOp*)node;
            struct Token op = o->operator;
            switch (op.type) {
                case T_MOV: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg* src = (struct ANodeReg*)(o->operand2);

                        ba_append_byte(&a->buf, 0x89); //register to memory, 0x8b is memory to register
                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | r_tbl[src->t.type] | rm_tbl[dst->t.type]);
                    } else if (o->operand1->type == ANODE_DEREF && o->operand2->type == ANODE_REG) {
                        //[0x89][01<reg>101][8-bit displacement] register to ebp memory with displacement
                        ba_append_byte(&a->buf, 0x89);

                        struct ANodeDeref* dst = (struct ANodeDeref*)(o->operand1);
                        struct ANodeReg* src = (struct ANodeReg*)(o->operand2);
                        struct ANodeReg* mem = (struct ANodeReg*)(dst->reg);
                        struct ANodeImm *imm = (struct ANodeImm*)(dst->imm);
                        if (mem->t.type != T_EBP) {
                            printf("Dereferencing only supported with ebp for now!\n");
                        }
                        int dis = get_double(imm->t);
                        if (dis < -128 || dis > 127) {
                            printf("32-bit displacements not supported!\n");
                        }
                        ba_append_byte(&a->buf, mod_tbl[MOD_01] | r_tbl[src->t.type] | rm_tbl[mem->t.type]);
                        if (dst->imm) {
                            ba_append_byte(&a->buf, get_byte(imm->t));
                        } else {
                            ba_append_byte(&a->buf, 0x00);
                        }
                    } else if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_DEREF) {
                        //[0x8b][01<reg>101][8-bit displacement] ebp memory with displacement to register
                        ba_append_byte(&a->buf, 0x8b);
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeDeref* src = (struct ANodeDeref*)(o->operand2);
                        struct ANodeReg *mem = (struct ANodeReg*)(src->reg);
                        struct ANodeImm *imm = (struct ANodeImm*)(src->imm);
                        if (mem->t.type != T_EBP) {
                            printf("Dereferencing only supported with ebp for now!\n");
                        }
                        int dis = get_double(imm->t);
                        if (dis < -128 || dis > 127) {
                            printf("32-bit displacements not supported!\n");
                        }
                        ba_append_byte(&a->buf, mod_tbl[MOD_01] | r_tbl[dst->t.type] | rm_tbl[mem->t.type]);
                        if (src->imm) {
                            ba_append_byte(&a->buf, get_byte(imm->t));
                        } else {
                            ba_append_byte(&a->buf, 0x00);
                        }
                    } else if (o->operand1->type == ANODE_REG) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        ba_append_byte(&a->buf, 0xb8 + dst->t.type);
                        assemble_node(a, o->operand2);
                    }
                    break;
                }
                case T_ADD: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg* src = (struct ANodeReg*)(o->operand2);
                        ba_append_byte(&a->buf, 0x01);
                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | r_tbl[src->t.type] | rm_tbl[dst->t.type]);
                    } else if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_IMM32) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        if (dst->t.type == T_EAX) {
                            ba_append_byte(&a->buf, 0x05);
                        } else {
                            ba_append_byte(&a->buf, 0x81);
                            ba_append_byte(&a->buf, mod_tbl[MOD_REG] | 0x00 << 3 | rm_tbl[dst->t.type]);
                        }
                        assemble_node(a, o->operand2);
                    }
                    break;
                }
                case T_SUB: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg* src = (struct ANodeReg*)(o->operand2);

                        ba_append_byte(&a->buf, 0x29);
                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | r_tbl[src->t.type] | rm_tbl[dst->t.type]);
                    } else if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_IMM32) {
                        struct ANodeReg *dst = (struct ANodeReg*)(o->operand1);
                        if (dst->t.type == T_EAX) {
                            ba_append_byte(&a->buf, 0x2d);
                        } else {
                            ba_append_byte(&a->buf, 0x81);
                            ba_append_byte(&a->buf, mod_tbl[MOD_REG] | 0x05 << 3 | rm_tbl[dst->t.type]);
                        }
                        assemble_node(a, o->operand2);
                    } 
                    break;
                }
                case T_IMUL: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        ba_append_byte(&a->buf, 0x0f);
                        ba_append_byte(&a->buf, 0xaf);

                        struct ANodeReg *dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg *src = (struct ANodeReg*)(o->operand2);

                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | r_tbl[dst->t.type] | rm_tbl[src->t.type]);
                    } else if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_IMM32) {
                        ba_append_byte(&a->buf, 0x69);

                        struct ANodeReg *dst = (struct ANodeReg*)(o->operand1);
                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | r_tbl[dst->t.type] | rm_tbl[dst->t.type]);

                        assemble_node(a, o->operand2);
                    }
                    break;
                }
                case T_IDIV: {
                    if (o->operand1->type == ANODE_REG && o->operand2 == NULL) {
                        //f7 /7 - sign divide edx:eax by r/m32, eax := quotient and edx := remainder
                        ba_append_byte(&a->buf, 0xf7);
                        struct ANodeReg* divisor = (struct ANodeReg*)(o->operand1);
                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | 0x07 << 3 | rm_tbl[divisor->t.type]);
                    } else {
                        //TODO: error message
                    }
                    break;
                }
                case T_PUSH: {
                    if (o->operand1->type == ANODE_IMM32 || o->operand1->type == ANODE_LABEL_REF) {
                        //68 id
                        ba_append_byte(&a->buf, 0x68);
                        assemble_node(a, o->operand1);
                    } else if (o->operand1->type == ANODE_REG) {
                        //ff /6
                        ba_append_byte(&a->buf, 0xff);
                        struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | 0x06 << 3 | rm_tbl[reg->t.type]);
                    }
                    break;
                }
                case T_POP: {
                    //8f /0 - pop stack into r/m32
                    //58 + rd - pop stack into r32
                    struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                    ba_append_byte(&a->buf, 0x58 + reg->t.type);
                    break;
                }
                case T_INTR: {
                    if (o->operand1->type == ANODE_IMM32) {
                        //cd ib
                        ba_append_byte(&a->buf, 0xcd);
                        struct ANodeImm* imm = (struct ANodeImm*)(o->operand1);
                        ba_append_byte(&a->buf, get_byte(imm->t));
                    } else {
                        printf("Operator not recognized: INTR branch\n");
                    }
                    break;
                }
                case T_ORG: {
                    if (o->operand1->type == ANODE_IMM32) {
                        struct ANodeImm* imm = (struct ANodeImm*)(o->operand1);
                        a->location = get_double(imm->t);
                    } else {
                        printf("Operand must be immediate value\n");
                    }
                    break;
                }
                case T_CDQ: {
                    if (o->operand1 == NULL && o->operand2 == NULL) {
                        ba_append_byte(&a->buf, 0x99);
                    } else {
                        printf("Operator not recognized: default case\n");
                    }
                    break;
                }
                case T_XOR: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        ba_append_byte(&a->buf, 0x33);

                        struct ANodeReg *dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg *src = (struct ANodeReg*)(o->operand2);

                        ba_append_byte(&a->buf, mod_tbl[MOD_REG] | r_tbl[dst->t.type] | rm_tbl[src->t.type]);
                    } else {
                        printf("XOR only works between registers for now\n");
                    }
                    break;
                }
                case T_CALL: {
                    if (o->operand1->type == ANODE_LABEL_REF && o->operand2 == NULL) {
                        ba_append_byte(&a->buf, 0xe8);
                        struct ALabel *label = NULL;
                        struct ANodeLabelRef* lr = (struct ANodeLabelRef*)(o->operand1);
                        if (!(label = ala_get_label(&a->ala, lr->t))) {
                            struct ALabel al;
                            al_init(&al, lr->t, 0, false);
                            ala_add(&a->ala, al);
                            label = ala_get_label(&a->ala, lr->t);
                        }
                        u32a_add(&label->rjmp_locs, a->buf.count);
                        ba_append_double(&a->buf, a->buf.count + 4); //put address of following instruction (patch displacement relative to this)
                    } else {
                        printf("CALL only works with labels for now\n");
                    }
                    break;
                }
                case T_JMP: {
                    if (o->operand1->type == ANODE_LABEL_REF && o->operand2 == NULL) {
                        ba_append_byte(&a->buf, 0xe9);
                        struct ALabel *label = NULL;
                        struct ANodeLabelRef* lr = (struct ANodeLabelRef*)(o->operand1);
                        if (!(label = ala_get_label(&a->ala, lr->t))) {
                            struct ALabel al;
                            al_init(&al, lr->t, 0, false);
                            ala_add(&a->ala, al);
                            label = ala_get_label(&a->ala, lr->t);
                        }
                        u32a_add(&label->rjmp_locs, a->buf.count);
                        ba_append_double(&a->buf, a->buf.count + 4); //put address of following instruction (patch displacement relative to this)
                    } else {
                        printf("JMP only works with labels for now\n");
                    }
                    break;
                }
                case T_JG: {
                    if (o->operand1->type == ANODE_LABEL_REF && o->operand2 == NULL) {
                        ba_append_byte(&a->buf, 0x0f);
                        ba_append_byte(&a->buf, 0x8f);
                        struct ALabel *label = NULL;
                        struct ANodeLabelRef* lr = (struct ANodeLabelRef*)(o->operand1);
                        if (!(label = ala_get_label(&a->ala, lr->t))) {
                            struct ALabel al;
                            al_init(&al, lr->t, 0, false);
                            ala_add(&a->ala, al);
                            label = ala_get_label(&a->ala, lr->t);
                        }
                        u32a_add(&label->rjmp_locs, a->buf.count);
                        ba_append_double(&a->buf, a->buf.count + 4); //put address of following instruction (patch displacement relative to this)
                    } else {
                        printf("JMP only works with labels for now\n");
                    }
                    break;
                }
                case T_CMP: {
                    //0x3d id <----cmp    eax, imm32
                    //0x81 /7 id <----- cmp   r/m32, imm32
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_IMM32) {
                        struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);

                        if (reg->t.type == T_EAX) {
                            ba_append_byte(&a->buf, 0x3d);
                            assemble_node(a, o->operand2);
                        } else {
                            ba_append_byte(&a->buf, 0x81);
                            ba_append_byte(&a->buf, mod_tbl[MOD_REG] | 0x07 << 3 | rm_tbl[reg->t.type]);
                            assemble_node(a, o->operand2);
                        }
                    } else {
                        printf("CMP only supports register, imm32 for now\n");
                    }
                    break;
                }
                case T_RET: {
                    ba_append_byte(&a->buf, 0xc3);
                    break;
                }
                default:
                    printf("Operator not recognized: default case\n");
                    break;
            }
            break;
        }
        default:
            printf("Not supported yet\n");
    }
}



void assembler_append_program(struct Assembler *a, struct NodeArray *na) {
    //a->program_start_loc = a->buf.count;

    for (int i = 0; i < na->count; i++) {
        assemble_node(a, na->nodes[i]);
    }

    for (int i = 0; i < a->ala.count; i++) {
        struct ALabel* l = &a->ala.elements[i];
        if (strncmp("_start", l->t.start, l->t.len) == 0) {
            if (!(l->defined)) {
                ems_add(&ems, l->t.line, "Assembling Error: Label '%.*s' not defined.", l->t.len, l->t.start);
            } else {
                a->program_start_loc = l->addr;
                break;
            }
        }
    }

    uint32_t filesz = a->buf.count;
    memcpy(&a->buf.bytes[a->filesz_patch], &filesz, 4);

    uint32_t memsz = a->buf.count;
    memcpy(&a->buf.bytes[a->memsz_patch], &memsz, 4);
}

void assembler_patch_locations(struct Assembler *a) {
    uint32_t program_start = a->program_start_loc + a->location;
    memcpy(&a->buf.bytes[a->program_start_patch], &program_start, 4);

    uint32_t vaddr = a->location;
    memcpy(&a->buf.bytes[a->vaddr_patch], &vaddr, 4);

    uint32_t paddr = a->location;
    memcpy(&a->buf.bytes[a->paddr_patch], &paddr, 4);
}

void assembler_patch_labels(struct Assembler *a) {
    for (int i = 0; i < a->ala.count; i++) {
        struct ALabel* l = &a->ala.elements[i];
        if (!(l->defined)) {
            ems_add(&ems, l->t.line, "Assembling Error: Label '%.*s' not defined.", l->t.len, l->t.start);
        } else {
            uint32_t final_addr = l->addr + a->location;
            for (int j = 0; j < l->ref_locs.count; j++) {
                uint32_t loc = l->ref_locs.elements[j];
                memcpy(&a->buf.bytes[loc], &final_addr, 4);
            }
        }
    }
}

void assembler_patch_rjmp(struct Assembler *a) {
    for (int i = 0; i < a->ala.count; i++) {
        struct ALabel *l = &a->ala.elements[i];
        if (!(l->defined)) {
            ems_add(&ems, l->t.line, "Assembling Error: Label '%.*s' not defined.", l->t.len, l->t.start);
        } else {
            for (int j = 0; j < l->rjmp_locs.count; j++) {
                uint32_t loc = l->rjmp_locs.elements[j];
                uint32_t *next = (uint32_t*)(&a->buf.bytes[loc]);
                uint32_t final_addr = l->addr - *next;
                memcpy(&a->buf.bytes[loc], &final_addr, 4);
            }
        }
    }
}

void assembler_write_binary(struct Assembler *a, char* filename) {
    FILE *f = fopen(filename, "wb");
    fwrite(a->buf.bytes, sizeof(uint8_t), a->buf.count, f);
    fclose(f);
}
