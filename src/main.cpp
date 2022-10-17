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
#include "translater.hpp"
#include "semant.hpp"

#define MAX_MSG_LEN 256

extern size_t allocated;

int main (int argc, char **argv) {

    ems_init(&ems);

    
    if (argc < 2) {
        printf("Usage: tama <filename>\n");
        exit(1);
    }

    Semant s;
    s.generate_asm(argv[1], "out.asm");

/*
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
    }*/

//    Translater translater;
//    translater.emit_asm(ast_nodes, "out.asm");

    Assembler assembler;
    assembler.emit_code("out.asm", "out.bin");
   
    ems_print(&ems);
    
    //cleanup
    printf("Memory allocated: %ld\n", allocated);
//    compiler_free(&c);
//    na_free(&na);
//    ta_free(&ta);
    ems_free(&ems);
    printf("Allocated memory remaining: %ld\n", allocated);



    return 0;
}
