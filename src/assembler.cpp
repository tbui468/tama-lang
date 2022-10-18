#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <utility>

#include "assembler.hpp"

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
    m_buf.push_back(0x7f);
    m_buf.push_back('E');
    m_buf.push_back('L');
    m_buf.push_back('F');
    m_buf.push_back(1);
    m_buf.push_back(1);
    m_buf.push_back(1);
    m_buf.push_back(0);

    
    uint8_t zeros[8] = {0};
    m_buf.insert(m_buf.end(), zeros, zeros + 8);
    uint16_t file_type = 2;
    m_buf.insert(m_buf.end(), (uint8_t*)&file_type, (uint8_t*)&file_type + sizeof(file_type));
    uint16_t arch_type = 3;
    m_buf.insert(m_buf.end(), (uint8_t*)&arch_type, (uint8_t*)&arch_type + sizeof(arch_type));
    uint32_t file_version = 1;
    m_buf.insert(m_buf.end(), (uint8_t*)&file_version, (uint8_t*)&file_version + sizeof(file_version));

    m_program_addr_offset = m_buf.size();     //need to add executable offset (0x08048000)
    m_buf.insert(m_buf.end(), zeros, zeros + 4);

    m_phdr_addr_offset = m_buf.size();
    m_buf.insert(m_buf.end(), zeros, zeros + 4);

    m_buf.insert(m_buf.end(), zeros, zeros + 8);

    m_ehdr_size_offset = m_buf.size();
    m_buf.insert(m_buf.end(), zeros, zeros + 2);

    m_phdr_size_offset = m_buf.size();
    m_buf.insert(m_buf.end(), zeros, zeros + 2);

    uint16_t phdr_entries = 1;
    m_buf.insert(m_buf.end(), (uint8_t*)&phdr_entries, (uint8_t*)&phdr_entries + sizeof(uint16_t));

    m_buf.insert(m_buf.end(), zeros, zeros + 6);

    uint16_t ehdr_size = m_buf.size();
    uint16_t *ptr = (uint16_t*)&m_buf[m_ehdr_size_offset];
    *ptr = ehdr_size;
}

void Assembler::append_program_header() {
    uint32_t phdr_start = m_buf.size();
    uint32_t *ptr = (uint32_t*)&m_buf[m_phdr_addr_offset];
    *ptr = phdr_start;

    uint32_t type = 1; //load
    m_buf.insert(m_buf.end(), (uint8_t*)&type, (uint8_t*)&type + 4);

    uint32_t offset = 0;
    m_buf.insert(m_buf.end(), (uint8_t*)&offset, (uint8_t*)&offset + 4);

    m_vaddr_offset = m_buf.size();
    uint32_t vaddr = 0;
    m_buf.insert(m_buf.end(), (uint8_t*)&vaddr, (uint8_t*)&vaddr + 4);

    m_paddr_offset = m_buf.size();
    uint32_t paddr = 0;
    m_buf.insert(m_buf.end(), (uint8_t*)&paddr, (uint8_t*)&paddr + 4);

    m_filesz_offset = m_buf.size();
    uint32_t filesz = 0;
    m_buf.insert(m_buf.end(), (uint8_t*)&filesz, (uint8_t*)&filesz + 4);

    m_memsz_offset = m_buf.size();
    uint32_t memsz = 0;
    m_buf.insert(m_buf.end(), (uint8_t*)&memsz, (uint8_t*)&memsz + 4);

    uint32_t flags = 5;
    m_buf.insert(m_buf.end(), (uint8_t*)&flags, (uint8_t*)&flags + 4);

    uint32_t align = 0x1000;
    m_buf.insert(m_buf.end(), (uint8_t*)&align, (uint8_t*)&align + 4);

    uint16_t phdr_size = m_buf.size() - phdr_start;
    uint16_t *phdrs_ptr = (uint16_t*)&m_buf[m_phdr_size_offset];
    *phdrs_ptr = phdr_size;
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
            m_program_start_addr = it->second.m_addr;
        }
    }

    uint32_t filesz = m_buf.size();
    uint32_t *ptr = (uint32_t*)&m_buf[m_filesz_offset];
    *ptr = filesz;

    uint32_t memsz = m_buf.size();
    uint32_t *ptr2 = (uint32_t*)&m_buf[m_memsz_offset];
    *ptr2 = memsz;
}

void Assembler::patch_addr_offsets() {
    uint32_t program_start = m_program_start_addr + m_load_addr;
    uint32_t *ptr = (uint32_t*)&m_buf[m_program_addr_offset];
    *ptr = program_start;

    uint32_t vaddr = m_load_addr;
    uint32_t *ptr2 = (uint32_t*)&m_buf[m_vaddr_offset];
    *ptr2 = vaddr;

    uint32_t paddr = m_load_addr;
    uint32_t *ptr3 = (uint32_t*)&m_buf[m_paddr_offset];
    *ptr3 = paddr;
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
