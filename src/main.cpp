#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "memory.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "assembler.hpp"
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


    Assembler assembler;
    assembler.emit_code("out.asm", "out.bin");
   
    ems_print(&ems);
    
    //cleanup
    printf("Memory allocated: %ld\n", allocated);
    ems_free(&ems);
    printf("Allocated memory remaining: %ld\n", allocated);

    return 0;
}
