#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <utility>

#include "assembler1.hpp"

void Assembler1::emit_code(const std::string& input_file, const std::string& output_file) {
    read(input_file);
    lex();
    parse();
    assemble();
    write(output_file);
}

void Assembler1::read(const std::string& input_file) {
    std::ifstream f(input_file);
    std::stringstream buffer;
    buffer << f.rdbuf();
    m_assembly = buffer.str();
}

void Assembler1::lex() {
    m_tokens = m_lexer.lex(m_assembly, m_reserved_words);
    for (struct Token t: m_tokens) {
        //printf("%.*s\n", t.len, t.start);
    }
}

Assembler1::Node *Assembler1::parse_operand() {
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
            return new NodeReg(next);
        case T_IDENTIFIER:
            return new NodeLabelRef(next);
        default:
            ems_add(&ems, next.line, "Parse Error: Unrecognized token!");
    }
    return NULL;
}

Assembler1::Node *Assembler1::parse_expr() {
    if (peek_one().type == T_L_BRACKET) {
        consume_token(T_L_BRACKET);
        struct Node* reg = parse_operand();
        if (peek_one().type == T_R_BRACKET) {
            consume_token(T_R_BRACKET);
            struct Token dummy;
            return new NodeMem(reg, dummy, NULL);
        } else if (peek_one().type == T_PLUS || peek_one().type == T_MINUS) {
            struct Token op = next_token();
            struct Node* displacement = parse_operand();
            consume_token(T_R_BRACKET);
            return new NodeMem(reg, op, displacement);
        }
    } else {
        return parse_operand();
    }
    ems_add(&ems, peek_one().line, "AParse Error: Invalid token type!");
    return NULL;
}

Assembler1::Node *Assembler1::parse_stmt() {
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
                left = parse_expr();
                consume_token(T_COMMA);
                right = parse_expr();
                break;
            //single operand
            case T_POP:
            case T_PUSH:
            case T_INTR:
            case T_ORG:
            case T_IDIV:
            case T_CALL:
            case T_JMP:
            case T_JG:
                left = parse_expr();
                break;
            //no operands
            case T_CDQ:
            case T_RET:
                break;
            default:
                ems_add(&ems, next.line, "AParse Error: Invalid token type!");
        }
        return new NodeOp(op, left, right);
    }
}

void Assembler1::parse() {
    
    while (peek_one().type != T_EOF) {
        m_nodes.push_back(parse_stmt());
    }

    for (Node* n: m_nodes) {
        //std::cout << "instruction: " << n->to_string() << '\n';
    }
}

struct Token Assembler1::peek_one() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Assembler1::peek_two() {
    int next = m_current + 1;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Assembler1::next_token() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    struct Token ret = m_tokens[next];
    m_current++;
    return ret;
}

struct Token Assembler1::consume_token(enum TokenType tt) {
    struct Token t = next_token();
    if (t.type != tt) {
        ems_add(&ems, t.line, "Unexpected token!");
    }
    return t;
}

bool Assembler1::end_of_tokens() {
    return m_current == m_tokens.size();
}

/*Translate assembly to machine code*/

void Assembler1::append_elf_header() {
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

void Assembler1::append_program_header() {
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

void Assembler1::append_program() {
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

void Assembler1::patch_addr_offsets() {
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

void Assembler1::patch_labels() {
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

void Assembler1::patch_rel_jumps() {
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

void Assembler1::assemble() {

    append_elf_header();
    append_program_header();
    append_program();
    patch_addr_offsets();
    patch_labels();
    patch_rel_jumps();
}

void Assembler1::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}
