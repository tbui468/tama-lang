#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <iostream>
#include <cassert>

#include "error.hpp"
#include "token.hpp"
#include "reserved_word.hpp"
#include "lexer.hpp"

class Assembler {
    public:
        //Switch to hex << 6 format for easier reading
        inline static std::array<uint8_t, 4> mod_tbl {{
            0x00,
            0x40,
            0x80,
            0xc0    //r/m is register
        }};

        enum class OpMod {
            MOD_00 = 0,
            MOD_01,
            MOD_10,
            MOD_REG
        };

        //registers table indices should match with enum TokenType register values
        //00xxx000
        inline static std::array<uint8_t, 8> r_tbl {{
            0x00 << 3,   //eax
            0x01 << 3,
            0x02 << 3,
            0x03 << 3,
            0x04 << 3,
            0x05 << 3,
            0x06 << 3,
            0x07 << 3
        }};

        //00000xxx
        inline static std::array<uint8_t, 8> rm_tbl {{
            0x00,   //eax
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0x06,
            0x07
        }};

        inline static std::vector<struct ReservedWordNew> m_reserved_words {{
            {"mov", T_MOV},
            {"push", T_PUSH},
            {"pop", T_POP},
            {"add", T_ADD},
            {"sub", T_SUB},
            {"imul", T_IMUL},
            {"div", T_DIV},
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
            {"jnz", T_JNZ},
            {"jg", T_JG},
            {"je", T_JE},
            {"test", T_TEST},
            {"cmp", T_CMP},
            {"neg", T_NEG},
            {"inc", T_INC},
            {"dec", T_DEC}
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
                virtual void assemble(Assembler& a) = 0;
                virtual std::string to_string() = 0;
                virtual int32_t eval() = 0;
        };

        class NodeOp: public Node {
            private:
                struct Token m_t;
                Node *m_left;
                Node *m_right;
            public:
                NodeOp(struct Token t, Node* left, Node* right): m_t(t), m_left(left), m_right(right) {}
                int32_t eval() {
                    assert(false && "NodeOp cannot call eval");
                }
                void assemble(Assembler& a) override {
                    switch(m_t.type) {
                        case T_ADD: {
                            if (dynamic_cast<NodeReg*>(m_left) && dynamic_cast<NodeReg*>(m_right)) {
                                a.m_buf.push_back(0x01);
                                NodeReg* dst = dynamic_cast<NodeReg*>(m_left);
                                NodeReg* src = dynamic_cast<NodeReg*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | r_tbl[src->m_t.type] | rm_tbl[dst->m_t.type]);
                            } else if (dynamic_cast<NodeReg*>(m_left) && is_expr(m_right)) {
                                NodeReg* reg = dynamic_cast<NodeReg*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    a.m_buf.push_back(0x05);
                                } else {
                                    a.m_buf.push_back(0x81);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x00 << 3 | rm_tbl[reg->m_t.type]);
                                }
                                m_right->assemble(a);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: add doesn't work with those operand types");
                            }
                            break;
                        }
                        case T_CALL: {
                            if (dynamic_cast<NodeLabelRef*>(m_left)) {
                                a.m_buf.push_back(0xe8);
                                m_left->assemble(a);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: call only works with labels for now");
                            }
                            break;
                        }
                        case T_CMP: {
                            //0x3d id <----cmp    eax, imm32
                            //0x81 /7 id <----- cmp   r/m32, imm32
                            if (dynamic_cast<NodeReg*>(m_left) && is_expr(m_right)) {
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    a.m_buf.push_back(0x3d);
                                    m_right->assemble(a);
                                } else {
                                    a.m_buf.push_back(0x81);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x07 << 3 | rm_tbl[reg->m_t.type]);
                                    m_right->assemble(a);
                                }
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: cmp only works imm32 for now");
                            }
                            break;
                        }
                        case T_DEC: {
                            if (dynamic_cast<NodeReg*>(m_left)) {
                                //ff /1 - DEC r/m32
                                a.m_buf.push_back(0xff);
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x01 << 3 | rm_tbl[reg->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: dec only works with registers");
                            }
                            break;
                        }
                        case T_DIV: {
                            //F7 /6 - DIV EDX:EAX by r/m32 with EAX := quotient and EDX := remainder
                            if (dynamic_cast<NodeReg*>(m_left)) {
                                a.m_buf.push_back(0xf7);
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x06 << 3 | rm_tbl[reg->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: div only works with register");
                            }
                            break;
                        }
                        case T_IMUL: {
                            if (dynamic_cast<NodeReg*>(m_left) && dynamic_cast<NodeReg*>(m_right)) {
                                //0F AF /r - IMUL r32, r/m32
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0xaf);
                                NodeReg* reg = dynamic_cast<NodeReg*>(m_left);
                                NodeReg* r_m = dynamic_cast<NodeReg*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | r_tbl[reg->m_t.type] | rm_tbl[r_m->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: SUB does not work with those operands.");
                            }
                            break;
                        }
                        case T_INC: {
                            if (dynamic_cast<NodeReg*>(m_left)) {
                                //ff /0 - INC r/m32
                                a.m_buf.push_back(0xff);
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | rm_tbl[reg->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: inc only works with register operands");
                            }
                            break;
                        }
                        case T_INTR: {
                            if (!is_expr(m_left)) {
                                ems_add(&ems, m_t.line, "Assembler Error: int operator must be followed by imm32.");
                            } else {
                                a.m_buf.push_back(0xcd);
                                a.m_buf.push_back((uint8_t)(m_left->eval())); //int instruction is followed by a single byte
                            }
                            break;
                        }
                        case T_JE: {
                            //0f 84 cd
                            a.m_buf.push_back(0x0f);
                            a.m_buf.push_back(0x84);
                            m_left->assemble(a);
                            break;
                        }
                        case T_JG: {
                            if (dynamic_cast<NodeLabelRef*>(m_left)) {
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x8f);
                                m_left->assemble(a);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: jg only works with labels for now");
                            }
                            break;
                        }
                        case T_JMP: {
                            if (dynamic_cast<NodeLabelRef*>(m_left)) {
                                a.m_buf.push_back(0xe9);
                                m_left->assemble(a);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: jmp only works with labels for now");
                            }
                            break;
                        }
                        case T_JNZ: {
                            if (dynamic_cast<NodeLabelRef*>(m_left)) {
                                //0F 85 cd
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x85);
                                m_left->assemble(a);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: jnz only works with labels for now");
                            }
                            break;
                        }
                        case T_MOV: {
                            if (dynamic_cast<NodeReg*>(m_left) && is_expr(m_right)) {
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                a.m_buf.push_back(0xb8 + reg->m_t.type);
                                m_right->assemble(a);
                            } else if (dynamic_cast<NodeReg*>(m_left) && dynamic_cast<NodeReg*>(m_right)) {
                                a.m_buf.push_back(0x89);
                                NodeReg *dst = dynamic_cast<NodeReg*>(m_left);
                                NodeReg *src = dynamic_cast<NodeReg*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | r_tbl[src->m_t.type] | rm_tbl[dst->m_t.type]);
                            } else if (dynamic_cast<NodeMem*>(m_left) && dynamic_cast<NodeReg*>(m_right)) {
                                //[0x89][01<reg>101][8-bit displacement] register to ebp memory with displacement
                                a.m_buf.push_back(0x89);

                                NodeMem *mem = dynamic_cast<NodeMem*>(m_left);
                                NodeReg *base = dynamic_cast<NodeReg*>(mem->m_base);
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_right);

                                if (base->m_t.type != T_EBP) {
                                    printf("Dereferencing only supported with ebp for now!\n");
                                }

                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_01] | r_tbl[reg->m_t.type] | rm_tbl[base->m_t.type]);

                                if (is_expr(mem->m_displacement)) {
                                    int32_t dis = mem->m_displacement->eval();
                                    if (dis < -128 || dis > 127) {
                                        printf("32-bit displacements not supported (yet)!\n");
                                    }
                                    a.m_buf.push_back((uint8_t)dis);
                                } else {
                                    a.m_buf.push_back(0x00);
                                }
                            } else if (dynamic_cast<NodeReg*>(m_left) && dynamic_cast<NodeMem*>(m_right)) {
                                //[0x8b][01<reg>101][8-bit displacement] ebp memory with displacement to register
                                a.m_buf.push_back(0x8b);

                                NodeReg* reg = dynamic_cast<NodeReg*>(m_left);
                                NodeMem* mem = dynamic_cast<NodeMem*>(m_right);
                                NodeReg* base = dynamic_cast<NodeReg*>(mem->m_base);

                                if (base->m_t.type != T_EBP) {
                                    printf("Dereferencing only supported with ebp for now!\n");
                                }

                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_01] | r_tbl[reg->m_t.type] | rm_tbl[base->m_t.type]);

                                if (is_expr(mem->m_displacement)) {
                                    int32_t dis = mem->m_displacement->eval();
                                    if (dis < -128 || dis > 127) {
                                        printf("32-bit displacements not supported!\n");
                                    }
                                    a.m_buf.push_back((uint8_t)dis);
                                } else {
                                    a.m_buf.push_back(0x00);
                                }
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: mov with those operands not supported");
                            }
                            break;
                        }
                        case T_NEG: {
                            if (dynamic_cast<NodeReg*>(m_left)) {
                                //F7 /3 - NEG r/m32
                                a.m_buf.push_back(0xf7);
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x03 << 3 | rm_tbl[reg->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: neg only accepts registers as operands");
                            }
                            break;
                        }
                        case T_ORG: {
                            if (!is_expr(m_left)) {
                                ems_add(&ems, m_t.line, "Assembler Error: org operator must be followed by imm32.");
                            } else {
                                a.m_load_addr = m_left->eval();
                            }
                            break;
                        }
                        case T_PUSH: {
                            if (is_expr(m_left)) {
                                //68 id
                                a.m_buf.push_back(0x68);
                                m_left->assemble(a);
                            } else if (dynamic_cast<NodeReg*>(m_left)) {
                                //ff /6
                                a.m_buf.push_back(0xff);
                                NodeReg* reg = dynamic_cast<NodeReg*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x06 << 3 | rm_tbl[reg->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: push with those operands not supported");
                            }
                            break;
                        }
                        case T_POP: {
                            //58 + rd - pop stack into r32
                            NodeReg* reg = dynamic_cast<NodeReg*>(m_left);
                            a.m_buf.push_back(0x58 + reg->m_t.type);
                            break;
                        }
                        case T_RET: {
                            a.m_buf.push_back(0xc3);
                            break;
                        }
                        case T_SUB: {
                            if (dynamic_cast<NodeReg*>(m_left) && dynamic_cast<NodeReg*>(m_right)) {
                                //29 /r - SUB r/m32, r32
                                NodeReg* r_m = dynamic_cast<NodeReg*>(m_left);
                                NodeReg* reg = dynamic_cast<NodeReg*>(m_right);
                                a.m_buf.push_back(0x29); 
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | r_tbl[reg->m_t.type] | rm_tbl[r_m->m_t.type]);
                            } else if (dynamic_cast<NodeReg*>(m_left) && is_expr(m_right)) {
                                NodeReg* reg = dynamic_cast<NodeReg*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    //2D id - SUB EAX, imm32
                                    a.m_buf.push_back(0x2d);
                                    m_right->assemble(a);
                                } else {
                                    //81 /5 id - SUB r/m32, imm32
                                    a.m_buf.push_back(0x81);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x5 << 3 | rm_tbl[reg->m_t.type]);
                                    m_right->assemble(a);
                                }
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: SUB does not work with those operands.");
                            }
                            break;
                        }
                        case T_TEST: {
                            if (dynamic_cast<NodeReg*>(m_left) && is_expr(m_right)) {
                                NodeReg *reg = dynamic_cast<NodeReg*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    //A9 id - TEST EAX, imm32
                                    a.m_buf.push_back(0xa9);
                                    m_right->assemble(a);
                                } else {
                                    //F7 /0 id - TEST r/m32, imm32
                                    a.m_buf.push_back(0xf7);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | rm_tbl[reg->m_t.type]);
                                    m_right->assemble(a);
                                }
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: test operator only works with [reg], [imm] for now");
                            }
                            break;
                        }
                        case T_XOR: {
                            if (dynamic_cast<NodeReg*>(m_left) && dynamic_cast<NodeReg*>(m_right)) {
                                a.m_buf.push_back(0x33);

                                NodeReg* dst = dynamic_cast<NodeReg*>(m_left);
                                NodeReg* src = dynamic_cast<NodeReg*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | r_tbl[dst->m_t.type] | rm_tbl[src->m_t.type]);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: xor only works with register operands for now.");
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

        class NodeReg: public Node {
            public:
                struct Token m_t;
            public:
                NodeReg(struct Token t): m_t(t) {}
                int32_t eval() {
                    assert(false && "NodeReg cannot call eval");
                }
                void assemble(Assembler& a) override {
                    //TODO
                }
                std::string to_string() {
                    return "Reg";
                }
        };

        class NodeImm: public Node {
            public:
                struct Token m_t;
            public:
                NodeImm(struct Token t): m_t(t) {}
                void assemble(Assembler& a) override {
                    uint32_t num = eval();
                    a.m_buf.insert(a.m_buf.end(), (uint8_t*)&num, (uint8_t*)&num + sizeof(uint32_t));
                }
                int32_t eval() {
                    return get_double(m_t);
                }
                std::string to_string() {
                    return "NodeImm";
                }
        };

        class NodeUnary: public Node {
            public:
                struct Token m_t;
                Node *m_right;
            public:
                NodeUnary(struct Token t, Node* right): m_t(t), m_right(right) {}
                void assemble(Assembler& a) override {
                    int32_t result = eval();
                    a.m_buf.insert(a.m_buf.end(), (uint8_t*)&result, (uint8_t*)&result + sizeof(int32_t));
                }
                int32_t eval() {
                    //assert that right is NodeImm, NodeUnary or NodeBinary
                    if (*m_t.start == '-') {
                        return -1 * m_right->eval();
                    }
                    
                    ems_add(&ems, m_t.line, "Assembler Error: Only '-' are recognized as unary operators");
                    return 0;
                }
                std::string to_string() {
                    return "NodeUnary";
                }
        };

        class NodeBinary: public Node {
            public:
                struct Token m_t;
                Node *m_left;
                Node *m_right;
            public:
                NodeBinary(struct Token t, Node *left, Node* right): m_t(t), m_left(left), m_right(right) {}
                void assemble(Assembler& a) override {
                    int32_t result = eval();
                    a.m_buf.insert(a.m_buf.end(), (uint8_t*)&result, (uint8_t*)&result + sizeof(int32_t));
                }
                int32_t eval() {
                    //assert that left/right are NodeImm, NodeUnary or NodeBinary
                    uint32_t left = m_left->eval();
                    uint32_t right = m_right->eval();
                    switch(m_t.type) {
                        case T_PLUS:    return left + right;
                        case T_MINUS:   return left - right;
                        case T_STAR:    return left * right;
                        case T_SLASH:   return left / right;
                        default:
                            ems_add(&ems, m_t.line, "Assembler Error: binary operator must be *+-/"); 
                            return 0;
                    }
                }
                std::string to_string() {
                    return "NodeBinary";
                }
        };

        class NodeLabelRef: public Node {
            public:
                struct Token m_t;
            public:
                NodeLabelRef(struct Token t): m_t(t) {}
                int32_t eval() {
                    assert(false && "NodeLabelRef cannot call eval");
                }
                void assemble(Assembler& a) override {
                    std::string s(m_t.start, m_t.len);
                    std::unordered_map<std::string, Label>::iterator it = a.m_labels.find(s);
                    if (it == a.m_labels.end()) {
                        a.m_labels.insert({s, Label(m_t, 0, false)});
                        it = a.m_labels.find(s);
                    }
                    it->second.m_rjmp_addr.push_back(a.m_buf.size());

                    uint32_t addr = a.m_buf.size() + 4;
                    a.m_buf.insert(a.m_buf.end(), (uint8_t*)&addr, (uint8_t*)&addr + sizeof(uint32_t));
                }
                std::string to_string() {
                    return "LabelRef";
                }
        };

        class NodeLabelDef: public Node {
            public:
                struct Token m_t;
            public:
                NodeLabelDef(struct Token t): m_t(t) {}
                int32_t eval() {
                    assert(false && "NodeLabelDef cannot be evaluated");
                }
                void assemble(Assembler& a) override {
                    std::string s(m_t.start, m_t.len);
                    std::unordered_map<std::string, Label>::iterator it = a.m_labels.find(s);
                    if (it != a.m_labels.end()) {
                        if (it->second.m_defined) {
                            ems_add(&ems, m_t.line, "Assembler Error: Labels cannot be defined more than once.");
                        } else {
                            it->second.m_t = m_t;
                            it->second.m_addr = a.m_buf.size();
                            it->second.m_defined = true;
                        }
                    } else {
                        a.m_labels.insert({s, Label(m_t, a.m_buf.size(), true)});
                    }
                }
                std::string to_string() {
                    return "LabelDef";
                }
        };

        class NodeMem: public Node {
            public:
                Node *m_base;
                Node *m_displacement;
            public:
                NodeMem(Node *base, Node *displacement): m_base(base), m_displacement(displacement) {}
                int32_t eval() {
                    assert(false && "NodeLabelDef cannot be evaluated");
                }
                void assemble(Assembler& a) override {
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
        Lexer m_lexer;
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
        Node *parse_unit();
        Node *parse_unary();
        Node *parse_factor();
        Node *parse_term();
        Node *parse_operand();
        Node *parse_stmt();
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

        static bool is_expr(Node *n) {
            return dynamic_cast<NodeImm*>(n) || dynamic_cast<NodeUnary*>(n) || dynamic_cast<NodeBinary*>(n);
        }
};


#endif //ASSEMBLER_HPP
