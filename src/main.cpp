#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "memory.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "byte_array.hpp"
#include "error.hpp"
#include "compiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"

#define MAX_MSG_LEN 256

extern size_t allocated;

char *load_code(char* filename) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); //rewind(f);
    char *string = (char*)malloc(fsize + 1);
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
//        compiler_output_assembly(&c);
    }



    Assembler assembler;
    assembler.emit_code("new_out.asm", "out.bin");
   
    ems_print(&ems);
    
    //cleanup
    printf("Memory allocated: %ld\n", allocated);
    compiler_free(&c);
    na_free(&na);
    ta_free(&ta);
    ems_free(&ems);
    printf("Allocated memory remaining: %ld\n", allocated);



    return 0;
}
