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
#include "linker.hpp"

#define MAX_MSG_LEN 256

extern size_t allocated;

int main (int argc, char **argv) {

    ems_init(&ems);

    
    if (argc < 2) {
        printf("Usage: tama <filename>\n");
        exit(1);
    }

    std::vector<std::string> tmd_files = std::vector<std::string>();
    std::vector<std::string> asm_files = std::vector<std::string>();
    std::vector<std::string> obj_files = std::vector<std::string>();

    for (int i = 1; i < argc; i++) {
        std::string s(argv[i]);
        if (s.ends_with(".tmd")) {
            tmd_files.push_back(s);
        } else if (s.ends_with(".asm")) {
            asm_files.push_back(s);
        } else if (s.ends_with(".obj")) {
            obj_files.push_back(s);
        } else {
            printf("Usage: only .tmd, .asm and .obj files recognized\n");
            exit(1);
        }
    }

    for (const std::string& f: tmd_files) {
        std::string out = f.substr(0, f.size() - 4) + ".asm";
        asm_files.push_back(out);

        std::cout << "Compiling " << f << " to IA32..." << std::endl;
        Semant s;
        s.generate_asm(f, out);
    }

    for (const std::string& f: asm_files) {
        std::string out = f.substr(0, f.size() - 4) + ".obj";
        obj_files.push_back(out);

        std::cout << "Assembling " << f << " to ELF relocatable objects..." << std::endl;
        Assembler a;
        a.generate_obj(f, out);
    }

    std::cout << "Linking ELF relocatable object(s) into ELF executable..." << std::endl;
    Linker l;
    l.link(obj_files, "out.exe");
   
    ems_print(&ems);

    if (ems.count > 0) return 1;
    
    //cleanup
//    printf("Memory allocated: %ld\n", allocated);
    ems_free(&ems);
//    printf("Allocated memory remaining: %ld\n", allocated);

    return 0;
}
