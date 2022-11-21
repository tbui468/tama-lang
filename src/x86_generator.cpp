#include <fstream>
#include <stdarg.h>
#include <iostream>
#include <unordered_map>

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

void X86Generator::fetch(const std::string& dst, const std::string& src) {
    if (is_int(src))   write_op("    %s     %s, %s", "mov", dst.c_str(), src.c_str());
    else               write_op("    %s     %s, [%s + %d]", "mov", dst.c_str(), "ebp", symbol_offset(src));
}


void X86Generator::store(const std::string& dst, const std::string& src) {
    write_op("    %s     [%s + %d], %s", "mov", "ebp", symbol_offset(dst), src.c_str());
}

int X86Generator::symbol_offset(const std::string& sym_name) {
    X86Frame current_frame = m_frames->find(m_frame_name)->second;
    Symbol* sym = current_frame.get_symbol_from_frame(sym_name);
    return sym->m_fp_offset;
}


void X86Generator::generate_asm(const ControlFlowGraph& cfg,
                                const std::vector<TacQuad>* quads, 
                                const std::vector<std::string>* labels, 
                                const std::unordered_map<std::string, X86Frame>* frames, 
                                const std::string& output_file) {

    m_frames = frames;

    for (const BasicBlock& bb: cfg.m_blocks) {
        for (int i = bb.m_begin; i < bb.m_end; i++) {
            TacQuad q = (*quads)[i];

            if (q.m_target == "" && q.m_opd1 == "" && q.m_opd2 == "" && q.m_op == T_NIL) {
                continue;
            }

            if ((*labels)[i] != "") {
                write_op("%s:", (*labels)[i].c_str());
            }

            if (q.m_op == T_CONDJUMP) {
                fetch("eax", q.m_target);
                write_op("    %s     %s, %s", "cmp", "eax", "0");
                write_op("    %s      %s", "je", q.m_opd1.c_str());
                if (q.m_opd2 != "") {
                    write_op("    %s     %s", "jmp", q.m_opd2.c_str());
                }
            } else if (q.m_target == "") {
                if (q.m_opd1 == "entry") {
                    write_op("    %s     %s, %s", "mov", "ebp", "esp");
                } else if (q.m_opd1 == "exit") {
                    write_op("    %s     %s, %s", "mov", "ebx", "eax");
                    write_op("    %s     %s, %s", "mov", "eax", "0x1");
                    write_op("    %s     %s", "int", "0x80");
                } else if (q.m_opd1 == "begin_fun") {
                    m_frame_name = (*labels)[i];
                    m_frame_size = q.m_opd2;
                    write_op("    %s    %s", "push", "ebp");
                    write_op("    %s     %s, %s", "mov", "ebp", "esp");
                    write_op("    %s     %s, %s", "sub", "esp", q.m_opd2.c_str());
                } else if (q.m_opd1 == "end_fun") {
                    m_frame_name = "";
                    m_frame_size = "";
                } else if (q.m_opd1 == "push_arg") {
                    if (is_int(q.m_opd2)) {
                        write_op("    %s    %s", "push", q.m_opd2.c_str());
                    } else {
                        fetch("eax", q.m_opd2);
                        write_op("    %s    %s", "push", "eax");
                    }
                } else if (q.m_opd1 == "pop_args") {
                    write_op("    %s     %s, %s", "add", "esp", q.m_opd2.c_str());
                } else if (q.m_opd1 == "call") {
                    write_op("    %s    %s", "call", q.m_opd2.c_str());
                } else if (q.m_opd1 == "goto") {
                    write_op("    %s     %s", "jmp", q.m_opd2.c_str());
                } else if (q.m_opd1 == "return") {
                    fetch("eax", q.m_opd2);
                    write_op("    %s     %s, %s", "add", "esp", m_frame_size.c_str());
                    write_op("    %s     %s", "pop", "ebp");
                    write_op("    %s", "ret");
                }
            } else {
                switch (q.m_op) {
                    case T_PLUS:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s     %s, %s", "add", "eax", "ecx");
                        store(q.m_target, "eax");
                        break;
                    case T_MINUS:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s     %s, %s", "sub", "eax", "ecx");
                        store(q.m_target, "eax");
                        break;
                    case T_STAR:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s    %s, %s", "imul", "eax", "ecx");
                        store(q.m_target, "eax");
                        break;
                    case T_SLASH:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s", "cdq");
                        write_op("    %s    %s", "idiv", "ecx");
                        store(q.m_target, "eax");
                        break;
                    case T_LESS:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s     %s, %s", "cmp", "eax", "ecx");
                        write_op("    %s    %s", "setl", "al");
                        write_op("    %s   %s, %s", "movzx", "eax", "al");
                        store(q.m_target, "eax");
                        break;
                    case T_EQUAL_EQUAL:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s     %s, %s", "cmp", "eax", "ecx");
                        write_op("    %s    %s", "sete", "al");
                        write_op("    %s   %s, %s", "movzx", "eax", "al");
                        store(q.m_target, "eax");
                        break;
                    case T_AND:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s     %s, %s", "and", "eax", "ecx");
                        store(q.m_target, "eax");
                        break;
                    case T_OR:
                        fetch("eax", q.m_opd1);
                        fetch("ecx", q.m_opd2);
                        write_op("    %s      %s, %s", "or", "eax", "ecx");
                        store(q.m_target, "eax");
                        break;
                    case T_EQUAL: {
                        if (q.m_opd1 == "call") {
                            write_op("    %s    %s", q.m_opd1.c_str(), q.m_opd2.c_str());
                            store(q.m_target, "eax");
                        } else {
                            fetch("eax", q.m_opd1);
                            store(q.m_target, "eax");
                        }
                        break;
                    }
                    default:
                        write_op("<not implemented>");
                        break;
                }
            }
            
        }
    }

    write(output_file);
}


void X86Generator::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}
