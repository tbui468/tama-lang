#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "memory.h"
#include "ast.h"
#include "token.h"
#include "byte_array.h"
#include "assembler.h"
#include "error.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"

#define MAX_MSG_LEN 256

extern size_t allocated;

char *load_code(char* filename) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); //rewind(f);
    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    return string;
}



int main (int argc, char **argv) {
    int TMD_COUNT = 11;
    struct ReservedWord tmd_reserved[11] = {
        {"print", 5, T_PRINT},
        {"int", 3, T_INT_TYPE},
        {"bool", 4, T_BOOL_TYPE},
        {"true", 4, T_TRUE},
        {"false", 5, T_FALSE},
        {"and", 3, T_AND},
        {"or", 2, T_OR},
        {"if", 2, T_IF},
        {"elif", 4, T_ELIF},
        {"else", 4, T_ELSE},
        {"while", 5, T_WHILE}
    };

    int ASM_COUNT = 20;
    struct ReservedWord asm_reserved[20] = {
        {"mov", 3, T_MOV},
        {"push", 4, T_PUSH},
        {"pop", 3, T_POP},
        {"add", 3, T_ADD},
        {"sub", 3, T_SUB},
        {"imul", 4, T_IMUL},
        {"idiv", 4, T_IDIV},
        {"eax", 3, T_EAX},
        {"ecx", 3, T_ECX},
        {"edx", 3, T_EDX},
        {"ebx", 3, T_EBX},
        {"esp", 3, T_ESP},
        {"ebp", 3, T_EBP},
        {"esi", 3, T_ESI},
        {"edi", 3, T_EDI},
        {"int", 3, T_INTR},
        {"equ", 3, T_EQU},
        {"org", 3, T_ORG},
        {"cdq", 3, T_CDQ},
        {"xor", 3, T_XOR}
    };


    ems_init(&ems);

    if (argc < 2) {
        printf("Usage: tama <filename>\n");
        exit(1);
    }
    char *code = load_code(argv[1]);

    //Tokenize source code
    struct Lexer l;
    lexer_init(&l, code, tmd_reserved, TMD_COUNT);

    struct TokenArray ta;
    ta_init(&ta);

    while (l.current < (int)strlen(l.code)) {
        struct Token t = lexer_next_token(&l);
        if (t.type != T_NEWLINE)
            ta_add(&ta, t);
        else
            l.line++;
    }

    struct Token t;
    t.type = T_EOF;
    t.start = NULL;
    t.len = 0;
    t.line = l.line;
    ta_add(&ta, t);

    for (int i = 0; i < ta.count; i++) {
//        printf("[%d] %.*s.  Type:%d\n", i, ta.tokens[i].len, ta.tokens[i].start, ta.tokens[i].type);
    }


    //Parse tokens into AST
    struct Parser p;
    parser_init(&p, &ta);
    struct NodeArray na;
    na_init(&na);
    
    while (parser_peek_one(&p).type != T_EOF) {
        na_add(&na, parse_stmt(&p));
    }


    for (int i = 0; i < na.count; i++) {
//        ast_print(na.nodes[i]);
//        printf("\n");
    }

    
    struct Compiler c;
    compiler_init(&c);

    compiler_begin_scope(&c);
    for (int i = 0; i < na.count; i++) {
        compiler_compile(&c, na.nodes[i]);
    }
    compiler_end_scope(&c);

    if (ems.count <= 0) {
        compiler_output_assembly(&c);
    }


    //Tokenize assembly
    char *acode = load_code("out.asm");
    struct Lexer al;
    lexer_init(&al, acode, asm_reserved, ASM_COUNT);

    struct TokenArray ata;
    ta_init(&ata);

    while (al.current < (int)strlen(al.code)) {
        struct Token t = lexer_next_token(&al);
        if (t.type != T_NEWLINE)
            ta_add(&ata, t);
        else
            al.line++;
    }

    struct Token at;
    at.type = T_EOF;
    at.start = NULL;
    at.len = 0;
    at.line = al.line;
    ta_add(&ata, at);

    //parse assembly
    struct Parser ap;
    parser_init(&ap, &ata);
    struct NodeArray ana;
    na_init(&ana);
    
    while (parser_peek_one(&ap).type != T_EOF) {
        na_add(&ana, aparse_stmt(&ap));
    }

    for (int i = 0; i < ana.count; i++) {
        //ast_print(ana.nodes[i]);
        //printf("\n");
    }

    //assemble
    struct Assembler a;
    assembler_init(&a);

    assembler_append_elf_header(&a);
    assembler_append_program_header(&a);  
    assembler_append_program(&a, &ana);
    assembler_patch_locations(&a);
    assembler_patch_labels(&a);
    
    assembler_write_binary(&a, "out.bin");


   
    ems_print(&ems);
    
    //cleanup
    printf("Memory allocated: %ld\n", allocated);
    compiler_free(&c);
    na_free(&na);
    ta_free(&ta);
    na_free(&ana);
    ta_free(&ata);
    assembler_free(&a);
    ems_free(&ems);
    printf("Allocated memory remaining: %ld\n", allocated);

    return 0;
}
