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
#include "elf.hpp"

class Assembler {
    public:

        inline static std::array<uint8_t, 4> mod_tbl {{
            0x00 << 6,
            0x01 << 6,
            0x02 << 6,
            0x03 << 6
        }};

        enum class OpMod {
            MOD_00 = 0,
            MOD_01,
            MOD_10,
            MOD_REG
        };


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
            {"al", T_AL},
            {"cl", T_CL},
            {"dl", T_DL},
            {"bl", T_BL},
            {"ah", T_AH},
            {"ch", T_CH},
            {"dh", T_DH},
            {"bh", T_BH},
            {"int", T_INTR},
            {"equ", T_EQU},
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
            {"dec", T_DEC},
            {"setl", T_SETL},
            {"movzx", T_MOVZX},
            {"or", T_OR},
            {"and", T_AND},
            {"setg", T_SETG},
            {"setle", T_SETLE},
            {"setge", T_SETGE},
            {"sete", T_SETE},
            {"setne", T_SETNE}
        }};

        class Label {
            public:
                Label(struct Token t, uint32_t addr, bool defined): 
                    m_t(t), m_addr(addr), m_defined(defined) {}
                struct Token m_t;
                uint32_t m_addr;
                bool m_defined;
                std::vector<uint32_t> m_rjmp_addr; //pc relative jumps (used in procedure calls)
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
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                a.m_buf.push_back(0x01);
                                NodeReg32* dst = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* src = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | src->bit_pattern() << 3 | dst->bit_pattern());
                            } else if (dynamic_cast<NodeReg32*>(m_left) && is_expr(m_right)) {
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    a.m_buf.push_back(0x05);
                                } else {
                                    a.m_buf.push_back(0x81);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x00 << 3 | reg->bit_pattern());
                                }
                                m_right->assemble(a);
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: add doesn't work with those operand types");
                            }
                            break;
                        }
                        case T_AND: {
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                //21 /r - AND r/m32, r32
                                a.m_buf.push_back(0x21);

                                NodeReg32* mem = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | reg->bit_pattern() << 3 | mem->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: and doesn't work with those operand types");
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
                        case T_CDQ: {
                            //99 CDQ - EDX:EAX := sign-extend of EAX
                            a.m_buf.push_back(0x99);
                            break;
                        }
                        case T_CMP: {
                            if (dynamic_cast<NodeReg32*>(m_left) && is_expr(m_right)) {
                                //3d id <----cmp    eax, imm32
                                //81 /7 id <----- cmp   r/m32, imm32
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    a.m_buf.push_back(0x3d);
                                    m_right->assemble(a);
                                } else {
                                    a.m_buf.push_back(0x81);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x07 << 3 | reg->bit_pattern());
                                    m_right->assemble(a);
                                }
                            } else if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                //39 /r - CMP r/32, r32
                                a.m_buf.push_back(0x39);

                                NodeReg32* mem = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | reg->bit_pattern() << 3 | mem->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: cmp does not work with those operands");
                            }
                            break;
                        }
                        case T_DEC: {
                            if (dynamic_cast<NodeReg32*>(m_left)) {
                                //ff /1 - DEC r/m32
                                a.m_buf.push_back(0xff);
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x01 << 3 | reg->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: dec only works with registers");
                            }
                            break;
                        }
                        case T_DIV: {
                            //F7 /6 - DIV EDX:EAX by r/m32 with EAX := quotient and EDX := remainder
                            if (dynamic_cast<NodeReg32*>(m_left)) {
                                a.m_buf.push_back(0xf7);
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x06 << 3 | reg->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: div only works with register");
                            }
                            break;
                        }
                        case T_IDIV: {
                            //F7 /7 - IDIV EDX:EAX by rm/32 with EAX := quotient and EDX := remainder
                            if (dynamic_cast<NodeReg32*>(m_left)) {
                                a.m_buf.push_back(0xf7);
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x07 << 3 | reg->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: div only works with register");
                            }
                            break;
                        }
                        case T_IMUL: {
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                //0F AF /r - IMUL r32, r/m32
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0xaf);
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* r_m = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | reg->bit_pattern() << 3 | r_m->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: SUB does not work with those operands.");
                            }
                            break;
                        }
                        case T_INC: {
                            if (dynamic_cast<NodeReg32*>(m_left)) {
                                //ff /0 - INC r/m32
                                a.m_buf.push_back(0xff);
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg->bit_pattern());
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
                            if (dynamic_cast<NodeReg32*>(m_left) && is_expr(m_right)) {
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(0xb8 + reg->m_t.type);
                                m_right->assemble(a);
                            } else if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                a.m_buf.push_back(0x89);
                                NodeReg32 *dst = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32 *src = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | src->bit_pattern() << 3 | dst->bit_pattern());
                            } else if (dynamic_cast<NodeMem*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                //[0x89][01<reg>101][8-bit displacement] register to ebp memory with displacement
                                a.m_buf.push_back(0x89);

                                NodeMem *mem = dynamic_cast<NodeMem*>(m_left);
                                NodeReg32 *base = dynamic_cast<NodeReg32*>(mem->m_base);
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_right);

                                if (base->m_t.type != T_EBP) {
                                    printf("Dereferencing only supported with ebp for now!\n");
                                }

                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_01] | reg->bit_pattern() << 3 | base->bit_pattern());

                                if (is_expr(mem->m_displacement)) {
                                    int32_t dis = mem->m_displacement->eval();
                                    if (dis < -128 || dis > 127) {
                                        printf("32-bit displacements not supported (yet)!\n");
                                    }
                                    a.m_buf.push_back((uint8_t)dis);
                                } else {
                                    a.m_buf.push_back(0x00);
                                }
                            } else if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeMem*>(m_right)) {
                                //[0x8b][01<reg>101][8-bit displacement] ebp memory with displacement to register
                                a.m_buf.push_back(0x8b);

                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                                NodeMem* mem = dynamic_cast<NodeMem*>(m_right);
                                NodeReg32* base = dynamic_cast<NodeReg32*>(mem->m_base);

                                if (base->m_t.type != T_EBP) {
                                    printf("Dereferencing only supported with ebp for now!\n");
                                }

                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_01] | reg->bit_pattern() << 3 | base->bit_pattern());

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
                        case T_MOVZX: {
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg8*>(m_right)) {
                                //0f b6 /r - MOVZX r32, r/m8 
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0xb6);
                                
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg8* mem = dynamic_cast<NodeReg8*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | reg->bit_pattern() << 3 | mem->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: movzx with those operands not supported");
                            }
                            break;
                        }
                        case T_NEG: {
                            if (dynamic_cast<NodeReg32*>(m_left)) {
                                //F7 /3 - NEG r/m32
                                a.m_buf.push_back(0xf7);
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x03 << 3 | reg->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: neg only accepts registers as operands");
                            }
                            break;
                        }
                        case T_OR: {
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                //09 /r - OR r/m32, r32
                                a.m_buf.push_back(0x09);

                                NodeReg32* mem = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | reg->bit_pattern() << 3 | mem->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: and doesn't work with those operand types");
                            }
                            break;
                        }
                        case T_PUSH: {
                            if (is_expr(m_left)) {
                                //68 id
                                a.m_buf.push_back(0x68);
                                m_left->assemble(a);
                            } else if (dynamic_cast<NodeReg32*>(m_left)) {
                                //ff /6
                                a.m_buf.push_back(0xff);
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x06 << 3 | reg->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: push with those operands not supported");
                            }
                            break;
                        }
                        case T_POP: {
                            //58 + rd - pop stack into r32
                            NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                            a.m_buf.push_back(0x58 + reg->m_t.type);
                            break;
                        }
                        case T_RET: {
                            a.m_buf.push_back(0xc3);
                            break;
                        }
                        case T_SETL: {
                            if (dynamic_cast<NodeReg8*>(m_left)) {
                                //0f 9c 
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x9c);
                                NodeReg8* reg8 = dynamic_cast<NodeReg8*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg8->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: setl with those operands not supported");
                            }
                            break;
                        }
                        case T_SETG: {
                            if (dynamic_cast<NodeReg8*>(m_left)) {
                                //0f 9f
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x9f);
                                NodeReg8* reg8 = dynamic_cast<NodeReg8*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg8->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: setg with those operands not supported");
                            }
                            break;
                        }
                        case T_SETLE: {
                            if (dynamic_cast<NodeReg8*>(m_left)) {
                                //0f 9e 
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x9e);
                                NodeReg8* reg8 = dynamic_cast<NodeReg8*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg8->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: setle with those operands not supported");
                            }
                            break;
                        }
                        case T_SETGE: {
                            if (dynamic_cast<NodeReg8*>(m_left)) {
                                //0f 9d
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x9d);
                                NodeReg8* reg8 = dynamic_cast<NodeReg8*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg8->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: setge with those operands not supported");
                            }
                            break;
                        }
                        case T_SETE: {
                            if (dynamic_cast<NodeReg8*>(m_left)) {
                                //0f 94
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x94);
                                NodeReg8* reg8 = dynamic_cast<NodeReg8*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg8->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: sete with those operands not supported");
                            }
                            break;
                        }
                        case T_SETNE: {
                            if (dynamic_cast<NodeReg8*>(m_left)) {
                                //0f 95
                                a.m_buf.push_back(0x0f);
                                a.m_buf.push_back(0x95);
                                NodeReg8* reg8 = dynamic_cast<NodeReg8*>(m_left);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg8->bit_pattern());
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: setne with those operands not supported");
                            }
                            break;
                        }
                        case T_SUB: {
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                //29 /r - SUB r/m32, r32
                                NodeReg32* r_m = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(0x29); 
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | reg->bit_pattern() << 3 | r_m->bit_pattern());
                            } else if (dynamic_cast<NodeReg32*>(m_left) && is_expr(m_right)) {
                                NodeReg32* reg = dynamic_cast<NodeReg32*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    //2D id - SUB EAX, imm32
                                    a.m_buf.push_back(0x2d);
                                    m_right->assemble(a);
                                } else {
                                    //81 /5 id - SUB r/m32, imm32
                                    a.m_buf.push_back(0x81);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x5 << 3 | reg->bit_pattern());
                                    m_right->assemble(a);
                                }
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: SUB does not work with those operands.");
                            }
                            break;
                        }
                        case T_TEST: {
                            if (dynamic_cast<NodeReg32*>(m_left) && is_expr(m_right)) {
                                NodeReg32 *reg = dynamic_cast<NodeReg32*>(m_left);
                                if (reg->m_t.type == T_EAX) {
                                    //A9 id - TEST EAX, imm32
                                    a.m_buf.push_back(0xa9);
                                    m_right->assemble(a);
                                } else {
                                    //F7 /0 id - TEST r/m32, imm32
                                    a.m_buf.push_back(0xf7);
                                    a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | 0x0 << 3 | reg->bit_pattern());
                                    m_right->assemble(a);
                                }
                            } else {
                                ems_add(&ems, m_t.line, "Assembler Error: test operator only works with [reg], [imm] for now");
                            }
                            break;
                        }
                        case T_XOR: {
                            if (dynamic_cast<NodeReg32*>(m_left) && dynamic_cast<NodeReg32*>(m_right)) {
                                a.m_buf.push_back(0x33);

                                NodeReg32* dst = dynamic_cast<NodeReg32*>(m_left);
                                NodeReg32* src = dynamic_cast<NodeReg32*>(m_right);
                                a.m_buf.push_back(mod_tbl[(uint8_t)OpMod::MOD_REG] | dst->bit_pattern() << 3 | src->bit_pattern());
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

        class NodeReg32: public Node {
            public:
                struct Token m_t;
            public:
                NodeReg32(struct Token t): m_t(t) {}
                int32_t eval() {
                    assert(false && "NodeReg32 cannot call eval");
                }
                void assemble([[maybe_unused]] Assembler& a) override {
                    //TODO
                }
                std::string to_string() {
                    return "Reg";
                }
                uint8_t bit_pattern() {
                    switch(m_t.type) {
                        case T_EAX: return 0;
                        case T_ECX: return 1;
                        case T_EDX: return 2;
                        case T_EBX: return 3;
                        case T_ESP: return 4;
                        case T_EBP: return 5;
                        case T_ESI: return 6;
                        case T_EDI: return 7;
                        default:
                            assert(false && "NodeReg32 has invalid token");
                            return 0;
                    }
                }
        };

        class NodeReg8: public Node {
            public:
                struct Token m_t;
            public:
                NodeReg8(struct Token t): m_t(t) {}
                int32_t eval() {
                    assert(false && "NodeReg8 cannot call eval");
                }
                void assemble([[maybe_unused]] Assembler& a) override {
                    //TODO
                }
                std::string to_string() {
                    return "Reg8";
                }
                uint8_t bit_pattern() {
                    switch(m_t.type) {
                        case T_AL: return 0;
                        case T_CL: return 1;
                        case T_DL: return 2;
                        case T_BL: return 3;
                        case T_AH: return 4;
                        case T_CH: return 5;
                        case T_DH: return 6;
                        case T_BH: return 7;
                        default:
                            assert(false && "NodeReg8 has invalid token");
                            return 0;
                    }
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
                void assemble([[maybe_unused]] Assembler& a) override {

                    //TODO
                }
                std::string to_string() {
                    return "Mem";
                }
        };
    public:
        std::string m_assembly = "";
        std::vector<struct Token> m_tokens = std::vector<struct Token>();
        uint32_t m_current = 0;
        std::vector<Node*> m_nodes = std::vector<Node*>();
        std::vector<uint8_t> m_buf = std::vector<uint8_t>();
        std::unordered_map<std::string, Label> m_labels = std::unordered_map<std::string, Label>();
        Lexer m_lexer;
    public:
        void generate_obj(const std::string& input_file, const std::string& output_file);
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

        void align_boundry_to(int bytes);

        int append_section_header(Elf32SectionHeader h);
        void append_text_section(int sh_text_offset, bool* undefined_globals);
        void append_shstrtab_section(int sh_shstrtab_offset);
        void append_symtab_section(int sh_symtab_offset, int sh_text_offset, const std::string& input_file);
        void append_strtab_section(int sh_strtab_offset, const std::string& input_file);
        void append_rel_section(int sh_rel_offset, int sh_text_offset, int sh_symtab_offset, int sh_strtab_offset);

        void append_program();
        void patch_rel_jumps();
        void write(const std::string& output_file);

        static bool is_expr(Node *n) {
            return dynamic_cast<NodeImm*>(n) || dynamic_cast<NodeUnary*>(n) || dynamic_cast<NodeBinary*>(n);
        }
};


#endif //ASSEMBLER_HPP
