#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <random>

#include "ast.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "assembler.hpp"
#include "semant.hpp"
#include "linker.hpp"
#include "x86_frame.hpp"
#include "x86_generator.hpp"
#include "optimizer.hpp"
#include "ControlFlowGraph.hpp"

int main (int argc, char **argv) {

    
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
        std::string out = f.substr(0, f.size() - 4) + ".tac";

        std::cout << "Compiling " << f << " to IR..." << std::endl;
        std::unordered_map<std::string, X86Frame> frames = std::unordered_map<std::string, X86Frame>();
        Semant s = Semant(&frames);
        s.generate_ir(f, out);
        if (ems.has_errors()) {
            ems.print();
            return 1;
        }

        std::cout << "Generating control-flow graph..." <<std::endl;
        ControlFlowGraph cfg;
        cfg.create_basic_blocks(s.m_quads, s.m_tac_labels);
        cfg.generate_graph(s.m_quads);

        std::cout << "Optimizing IR..." << std::endl;
        Optimizer opt;
        opt.eliminate_dead_code(&cfg, s.m_tac_labels);
        opt.collapse_cond_jumps(&s.m_quads, &s.m_tac_labels);
        opt.fold_constants(&s.m_quads);
        opt.merge_adjacent_store_fetch(&s.m_quads);
        opt.simplify_algebraic_identities(&s.m_quads);


        std::cout << "Generating x86 code..." << std::endl;
        X86Generator gen;
        gen.generate_asm(cfg, &s.m_quads, &s.m_tac_labels, &frames, f.substr(0, f.size() - 4) + ".asm");
        asm_files.push_back(f.substr(0, f.size() - 4) + ".asm");

        if (ems.has_errors()) {
            ems.print();
            return 1;
        }
    }

    

    for (const std::string& f: asm_files) {
        std::string out = f.substr(0, f.size() - 4) + ".obj";
        obj_files.push_back(out);

        std::cout << "Assembling " << f << " to ELF relocatable objects..." << std::endl;
        Assembler a;
        a.generate_obj(f, out);

        if (ems.has_errors()) {
            ems.print();
            return 1;
        }
    }

    std::cout << "Linking ELF relocatable object(s) into ELF executable..." << std::endl;
    Linker l;
    l.link(obj_files, "out.exe");
   
    if (ems.has_errors()) {
        ems.print();
        return 1;
    }
    
    return 0;
}
