#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <string>
#include <vector>
#include <array>
#include <unordered_map>

#include "error.hpp"
#include "token.hpp"
#include "reserved_word.hpp"
#include "lexer_new.hpp"

/*
org     0x08048000

_start:
    mov     ebx, 42
    mov     eax, 0x01
    int     0x80
*/

class Assembler1 {
    public:
        inline static std::vector<struct ReservedWordNew> m_reserved_words {{
            {"mov", T_MOV},
            {"push", T_PUSH},
            {"pop", T_POP},
            {"add", T_ADD},
            {"sub", T_SUB},
            {"imul", T_IMUL},
            {"idiv", T_IDIV},
            {"eax", T_EAX},
            {"ecx", T_ECX},
            {"edx", T_EDX},
            {"ebx", T_EBX},
            {"esp", T_ESP},
            {"ebp", T_EBP},
            {"esi", T_ESI},
            {"edi", T_EDI},
            {"int", T_INTR},
            {"equ", T_EQU},
            {"org", T_ORG},
            {"cdq", T_CDQ},
            {"xor", T_XOR},
            {"call", T_CALL},
            {"ret", T_RET},
            {"jmp", T_JMP},
            {"jg", T_JG},
            {"cmp", T_CMP}
        }};

        class Label {
            public:
                Label(struct Token t, uint32_t addr, bool defined): 
                    m_t(t), m_addr(addr), m_defined(defined), m_ref_addr(std::vector<uint32_t>()), 
                    m_rjmp_addr(std::vector<uint32_t>()) {}
                struct Token m_t;
                uint32_t m_addr;
                bool m_defined;
                std::vector<uint32_t> m_ref_addr;
                std::vector<uint32_t> m_rjmp_addr;
        };

        class Node {
            public:
                virtual void assemble(Assembler1& a) = 0;
                virtual std::string to_string() = 0;
        };

        class NodeOp: public Node {
            private:
                struct Token m_t;
                Node *m_left;
                Node *m_right;
            public:
                NodeOp(struct Token t, Node* left, Node* right): m_t(t), m_left(left), m_right(right) {}
                void assemble(Assembler1& a) override {
                    switch(m_t.type) {
                        case T_ORG: {
                            NodeImm* ptr;
                            if (!(ptr = dynamic_cast<NodeImm*>(m_left))) {
                                ems_add(&ems, m_t.line, "Assembler Error: org operator must be followed by imm32.");
                            } else {
                                a.m_load_addr = get_double(ptr->m_t);
                            }
                            break;
                        }
                        default:
                            ems_add(&ems, m_t.line, "Assembler Error: operator not currently supported.");
                            break;
                    }
                }
                std::string to_string() {
                    std::string ret = "Op ";
                    if (m_left)
                        ret += m_left->to_string();
                    if (m_right)
                        ret += ", " + m_right->to_string();
                    return ret;
                }
        };

        class NodeImm: public Node {
            public:
                struct Token m_t;
            public:
                NodeImm(struct Token t): m_t(t) {}
                void assemble(Assembler1& a) override {
                    uint32_t num = get_double(m_t);
                    a.m_buf.insert(a.m_buf.end(), (uint8_t*)&num, (uint8_t*)&num + sizeof(uint32_t));
                }
                std::string to_string() {
                    return "Imm";
                }
        };

        class NodeReg: public Node {
            private:
                struct Token m_t;
            public:
                NodeReg(struct Token t): m_t(t) {}
                void assemble(Assembler1& a) override {
                    //TODO
                }
                std::string to_string() {
                    return "Reg";
                }
        };

        class NodeLabelRef: public Node {
            private:
                struct Token m_t;
            public:
                NodeLabelRef(struct Token t): m_t(t) {}
                void assemble(Assembler1& a) override {
                    //TODO: fill this in 
                }
                std::string to_string() {
                    return "LabelRef";
                }
        };

        class NodeLabelDef: public Node {
            private:
                struct Token m_t;
            public:
                NodeLabelDef(struct Token t): m_t(t) {}
                void assemble(Assembler1& a) override {
                    //TODO
                    //look for label inside of m_labels
                    //  if exists
                    //      if already defined, emit error
                    //      else
                    //          set token (just so token is definition rather than reference)
                    //          set address to a.m_buf.size()
                    //          set m_defined of label to true
                    //  else
                    //      add new label with defined = true
                    /*
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
                    }*/
                }
                std::string to_string() {
                    return "LabelDef";
                }
        };

        class NodeMem: public Node {
            private:
                Node *m_base;
                struct Token m_op;
                Node *m_displacement;
            public:
                NodeMem(Node *base, struct Token op, Node *displacement): m_base(base), m_op(op), m_displacement(displacement) {}
                void assemble(Assembler1& a) override {
                    //TODO
                }
                std::string to_string() {
                    return "Mem";
                }
        };
    public:
        std::string m_assembly = "";
        std::vector<struct Token> m_tokens = std::vector<struct Token>();
        int m_current = 0;
        std::vector<Node*> m_nodes = std::vector<Node*>();
        std::vector<uint8_t> m_buf = std::vector<uint8_t>();
        std::unordered_map<std::string, Label> m_labels = std::unordered_map<std::string, Label>();
        LexerNew m_lexer;
        uint32_t m_program_addr_offset = 0;
        uint32_t m_phdr_addr_offset = 0;
        uint32_t m_ehdr_size_offset = 0;
        uint32_t m_phdr_size_offset = 0;
        uint32_t m_vaddr_offset = 0;
        uint32_t m_paddr_offset = 0;
        uint32_t m_filesz_offset = 0;
        uint32_t m_memsz_offset = 0;
        uint32_t m_program_start_addr = 0;
        uint32_t m_load_addr = 0;
    public:
        void emit_code(const std::string& input_file, const std::string& output_file);
    private:
        void read(const std::string& input_file);
        void lex();

        void parse();
        Node* parse_expr();
        Node* parse_operand();
        Node* parse_stmt();
        struct Token peek_one();
        struct Token peek_two();
        struct Token next_token();
        struct Token consume_token(enum TokenType tt);
        bool end_of_tokens();

        void assemble();
        void append_elf_header();
        void append_program_header();
        void append_program();
        void patch_addr_offsets();
        void patch_labels();
        void patch_rel_jumps();
        void write(const std::string& output_file);
};


#endif //ASSEMBLER_HPP
