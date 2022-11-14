#include <fstream>
#include <stdarg.h>
#include <iostream>

#include "x86_generator.hpp"
#include "utility.hpp"

void X86Generator::write_op(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char s[256];
    int n = vsprintf(s, format, ap);
    va_end(ap);
    s[n] = '\n';
    s[n + 1] = '\0';
    std::string str(s);
    m_buf.insert(m_buf.end(), (uint8_t*)str.data(), (uint8_t*)str.data() + str.size());
}


void X86Generator::generate_asm(const std::vector<TacQuad>* quads, 
                                const std::vector<std::string>* labels, 
                                const std::unordered_map<std::string, X86Frame>* frames, 
                                const std::string& output_file) {

    /*
    for (const std::pair<std::string, X86Frame>& p: *frames) {
        std::cout << p.first << std::endl;
        for (const std::pair<std::string, int>& p2: p.second.m_fp_offsets) {
            std::cout << p2.first << std::endl; 
        }
    }*/

    std::string current_frame_name = "";
    int i = 0;
    for (const TacQuad& q: *quads) {
        std::cout << q.m_target << " " << q.m_opd1 << " " << q.m_opd2 << " \n";
        if ((*labels)[i] != "") {
            write_op("%s:", (*labels)[i].c_str());
        }


        if (q.m_target == "") {
            if (q.m_opd1 == "begin_fun") {
                current_frame_name = (*labels)[i];
                write_op("    %s    %s", "push", "ebp");
                write_op("    %s     %s, %s", "sub", "esp", q.m_opd2.c_str());
            } else if (q.m_opd1 == "push_arg") {
                if (is_int(q.m_opd2)) {
                    write_op("    %s    %s", "push", q.m_opd2.c_str());
                } else {
                    X86Frame current_frame = frames->find(current_frame_name)->second;
                    Symbol* sym = current_frame.get_symbol_from_frame(q.m_opd2);
                    write_op("    %s    [%s + %d]", "push", "ebp", sym->m_fp_offset);
                }
            } else if (q.m_opd1 == "pop_args") {
                write_op("    %s     %s, %s", "add", "esp", q.m_opd2.c_str());
            } else if (q.m_opd1 == "return") {
                //one with no operand
                //one with a single operand
            } else if (q.m_opd1 == "end_fun") {
                //clean the stack of locals
                //pop old base pointer back into ebp
                current_frame_name = "";
            } else if (q.m_opd1 == "call") {

            } else if (q.m_opd1 == "goto") {
                write_op("    %s     %s", "jmp", q.m_opd2.c_str());
            } else if (q.m_target != "" && q.m_opd1 != "" && q.m_opd2 != "") {
                //find location of target in frame
                //evaluate right side using registers
                //find location of target in frame
                //assign that memory location to target
            } 
        }
        //TODO: check each type of quad and writ out x86 using write_op
        
        i++;
    }
    write(output_file);
}


void X86Generator::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}
