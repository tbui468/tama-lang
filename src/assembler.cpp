#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <utility>

#include "assembler.hpp"

void Assembler::generate_obj(const std::string& input_file, const std::string& output_file) {
    /*
    read(input_file);
    lex();
    if (ems.count > 0) return;
    parse();
    if (ems.count > 0) return;

    append_relocatable_elf_header();
    append_program();
    append_section_header_table();

    if (ems.count > 0) return;
    write(output_file);*/
}


void Assembler::append_elf_header2() {
    Elf32ElfHeader eh;
    m_buf.insert(m_buf.end(), (uint8_t*)&eh, (uint8_t*)&eh + sizeof(Elf32ElfHeader));

    ((Elf32ElfHeader*)(m_buf.data()))->m_ehsize = m_buf.size();
}


void Assembler::emit_code(const std::string& input_file, const std::string& output_file) {
    read(input_file);
    lex();
    if (ems.count > 0) return;
    parse();
    if (ems.count > 0) return;
    assemble();
    if (ems.count > 0) return;
    write(output_file);
}

void Assembler::read(const std::string& input_file) {
    std::ifstream f(input_file);
    std::stringstream buffer;
    buffer << f.rdbuf();
    m_assembly = buffer.str();
}

void Assembler::lex() {
    m_tokens = m_lexer.lex(m_assembly, m_reserved_words);
    for (struct Token t: m_tokens) {
//        printf("%.*s\n", t.len, t.start);
    }
}

Assembler::Node *Assembler::parse_unit() {
    struct Token next = next_token();
    switch (next.type) {
        case T_INT:
        case T_HEX:
            return new NodeImm(next);
        case T_EAX:
        case T_ECX:
        case T_EDX:
        case T_EBX:
        case T_ESP:
        case T_EBP:
        case T_ESI:
        case T_EDI:
            return new NodeReg32(next);
        case T_AL:
        case T_CL:
        case T_DL:
        case T_BL:
        case T_AH:
        case T_CH:
        case T_DH:
        case T_BH:
            return new NodeReg8(next);
        case T_IDENTIFIER:
            return new NodeLabelRef(next);
        case T_L_BRACKET: {
            Node* reg = parse_unit();
            if (!dynamic_cast<NodeReg32*>(reg)) {
                ems_add(&ems, next.line, "Parse Error: Memory access requires register before displacement");
            }
            if (peek_one().type == T_R_BRACKET) {
                consume_token(T_R_BRACKET);
                return new NodeMem(reg, NULL);
            } else if (peek_one().type == T_PLUS) {
                next_token(); //Skip '+'
                Node* displacement = parse_operand();
                consume_token(T_R_BRACKET);
                return new NodeMem(reg, displacement);
            } else if (peek_one().type == T_MINUS) {
                Node *displacement = parse_operand();
                consume_token(T_R_BRACKET);
                return new NodeMem(reg, displacement);
            }
        }
        default:
            ems_add(&ems, next.line, "Parse Error: Unrecognized token!");
    }
    return NULL;
}

Assembler::Node *Assembler::parse_unary() {
    if (peek_one().type == T_MINUS) {
        struct Token op = next_token();
        return new NodeUnary(op, parse_unary());
    } else {
        return parse_unit();
    }
}

Assembler::Node *Assembler::parse_factor() {
    Node *left = parse_unary();
    while (peek_one().type == T_STAR || peek_one().type == T_SLASH) {
        struct Token op = next_token();
        left = new NodeBinary(op, left, parse_unary());
    }
    return left;
}

Assembler::Node *Assembler::parse_term() {
    Node *left = parse_factor();
    while (peek_one().type == T_PLUS || peek_one().type == T_MINUS) {
        struct Token op = next_token();
        left = new NodeBinary(op, left, parse_factor());
    }
    return left;
}

Assembler::Node *Assembler::parse_operand() {
    Node *operand = parse_term();
    return operand;
}

Assembler::Node *Assembler::parse_stmt() {
    struct Token next = peek_one();
    //if identifer followed by a colon, then it's a label
    if (next.type == T_IDENTIFIER && peek_two().type == T_COLON) {
        struct Token id = next_token();
        consume_token(T_COLON);
        return new NodeLabelDef(id);
    } else {
        struct Token op =  next_token();
        struct Node *left = NULL;
        struct Node *right = NULL;
        switch (op.type) {
            //two operands
            case T_MOV:
            case T_ADD:
            case T_SUB:
            case T_IMUL:
            case T_XOR:
            case T_CMP:
            case T_TEST:
            case T_MOVZX:
            case T_AND:
            case T_OR:
                left = parse_operand();
                consume_token(T_COMMA);
                right = parse_operand();
                break;
            //single operand
            case T_POP:
            case T_PUSH:
            case T_INTR:
            case T_ORG:
            case T_IDIV:
            case T_DIV:
            case T_CALL:
            case T_JMP:
            case T_JNZ:
            case T_JE:
            case T_JG:
            case T_NEG:
            case T_INC:
            case T_DEC:
            case T_SETL:
            case T_SETG:
            case T_SETLE:
            case T_SETGE:
            case T_SETE:
            case T_SETNE:
                left = parse_operand();
                break;
            //no operands
            case T_CDQ:
            case T_RET:
                break;
            default:
                ems_add(&ems, next.line, "Parse Error: Invalid token type!");
        }
        return new NodeOp(op, left, right);
    }
}

void Assembler::parse() {
    
    while (peek_one().type != T_EOF) {
        m_nodes.push_back(parse_stmt());
    }

    for (Node* n: m_nodes) {
//        std::cout << "instruction: " << n->to_string() << '\n';
    }
}

struct Token Assembler::peek_one() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Assembler::peek_two() {
    int next = m_current + 1;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Assembler::next_token() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    struct Token ret = m_tokens[next];
    m_current++;
    return ret;
}

struct Token Assembler::consume_token(enum TokenType tt) {
    struct Token t = next_token();
    if (t.type != tt) {
        ems_add(&ems, t.line, "Unexpected token!");
    }
    return t;
}

bool Assembler::end_of_tokens() {
    return m_current == m_tokens.size();
}

/*Translate assembly to machine code*/

void Assembler::append_elf_header() {
    Elf32ElfHeader eh;
    m_buf.insert(m_buf.end(), (uint8_t*)&eh, (uint8_t*)&eh + sizeof(Elf32ElfHeader));

    ((Elf32ElfHeader*)(m_buf.data()))->m_ehsize = m_buf.size();

}

void Assembler::append_program_header() {
    Elf32ElfHeader *eh = (Elf32ElfHeader*)m_buf.data();
    ((Elf32ElfHeader*)m_buf.data())->m_phoff = m_buf.size();

    Elf32ProgramHeader ph;
    m_buf.insert(m_buf.end(), (uint8_t*)&ph, (uint8_t*)&ph + sizeof(Elf32ProgramHeader));

    ((Elf32ElfHeader*)m_buf.data())->m_phentsize = m_buf.size() - eh->m_phoff;

}

void Assembler::append_program() {
    for (Node *n: m_nodes) {
        n->assemble(*this);
    }

    std::unordered_map<std::string, Label>::iterator it = m_labels.find("_start");
    if (it != m_labels.end()) {
        if (!it->second.m_defined) {
            ems_add(&ems, it->second.m_t.line, "Assembling Error: Label '%.*s' not defined.", it->second.m_t.len, it->second.m_t.start);
        } else {
            ((Elf32ElfHeader*)m_buf.data())->m_entry = it->second.m_addr;
        }
    }


    Elf32ProgramHeader *ph = (Elf32ProgramHeader*)(m_buf.data() + ((Elf32ElfHeader*)m_buf.data())->m_phoff);
    ph->m_filesz = m_buf.size();
    ph->m_memsz = m_buf.size();
}

void Assembler::patch_addr_offsets() {
    Elf32ElfHeader *eh = (Elf32ElfHeader*)m_buf.data();

    eh->m_entry += m_load_addr;

    Elf32ProgramHeader *ph = (Elf32ProgramHeader*)(m_buf.data() + eh->m_phoff);
    ph->m_vaddr = m_load_addr;
    ph->m_paddr = m_load_addr;
}

void Assembler::patch_labels() {
    for(const std::pair<std::string, Label>& it: m_labels) {
        const Label* l = &it.second;
        if (!l->m_defined) {
            ems_add(&ems, l->m_t.line, "Assembling Error: Label '%.*s' not defined.", l->m_t.len, l->m_t.start);
        } else {
            uint32_t final_addr = l->m_addr + m_load_addr;
            for (uint32_t addr: l->m_ref_addr) {
                uint32_t *ptr = (uint32_t*)&m_buf[addr];
                *ptr = final_addr;
            }
        }
    }
}

void Assembler::patch_rel_jumps() {
    for(const std::pair<std::string, Label>& it: m_labels) {
        const Label* l = &it.second;
        if (!l->m_defined) {
            ems_add(&ems, l->m_t.line, "Assembling Error: Label '%.*s' not defined.", l->m_t.len, l->m_t.start);
        } else {
            for (uint32_t addr: l->m_rjmp_addr) {
                uint32_t *ptr = (uint32_t*)&m_buf[addr];
                uint32_t final_addr = l->m_addr - *ptr;
                *ptr = final_addr;
            }
        }
    }
}

void Assembler::assemble() {
    append_elf_header();
    append_program_header();
    append_program();
    patch_addr_offsets();
    patch_labels();
    patch_rel_jumps();
}

void Assembler::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}
