#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <utility>

#include "assembler.hpp"

void Assembler::align_boundry_to(int bytes) {
    while (m_buf.size() % bytes != 0) {
        m_buf.push_back(0x0);
    }
}


void Assembler::append_text_section(int sh_text_offset, bool* undefined_globals) {
    align_boundry_to(16);
    
    ((Elf32SectionHeader*)(m_buf.data() + sh_text_offset))->m_offset = m_buf.size();
    append_program(); 
    ((Elf32SectionHeader*)(m_buf.data() + sh_text_offset))->m_size = m_buf.size() - ((Elf32SectionHeader*)(m_buf.data() + sh_text_offset))->m_offset;


    for(const std::pair<std::string, Label>& it: m_labels) {
        const Label* l = &it.second;
        if (!l->m_defined) {
            *undefined_globals = true;
            break;
        }
    }

    if (*undefined_globals) {
        ((Elf32ElfHeader*)m_buf.data())->m_shnum = 6; 
    } else {
        ((Elf32ElfHeader*)m_buf.data())->m_shnum = 5;
    }
}

void Assembler::append_shstrtab_section(int sh_shstrtab_offset) {
    align_boundry_to(1); //no alignment

    ((Elf32SectionHeader*)&m_buf[sh_shstrtab_offset])->m_offset = m_buf.size();

    m_buf.push_back('\0');

    std::string text_str = ".text";
    m_buf.insert(m_buf.end(), text_str.data(), text_str.data() + text_str.size());
    m_buf.push_back('\0');

    std::string shstrtab_str = ".shstrtab";
    m_buf.insert(m_buf.end(), shstrtab_str.data(), shstrtab_str.data() + shstrtab_str.size());
    m_buf.push_back('\0');

    std::string symtab_str = ".symtab";
    m_buf.insert(m_buf.end(), symtab_str.data(), symtab_str.data() + symtab_str.size());
    m_buf.push_back('\0');

    std::string strtab_str = ".strtab";
    m_buf.insert(m_buf.end(), strtab_str.data(), strtab_str.data() + strtab_str.size());
    m_buf.push_back('\0');

    std::string rel_str = ".rel.text";
    m_buf.insert(m_buf.end(), rel_str.data(), rel_str.data() + rel_str.size());
    m_buf.push_back('\0');

    ((Elf32SectionHeader*)&m_buf[sh_shstrtab_offset])->m_size = m_buf.size() - ((Elf32SectionHeader*)&m_buf[sh_shstrtab_offset])->m_offset;
}

void Assembler::append_symtab_section(int sh_symtab_offset, int sh_text_offset, const std::string& input_file) {
    align_boundry_to(4);

    ((Elf32SectionHeader*)(m_buf.data() + sh_symtab_offset))->m_offset = m_buf.size();

    Elf32Symbol sym_null;
    sym_null.m_name = 0;
    sym_null.m_value = 0;
    sym_null.m_size = 0;
    sym_null.m_info = 0;
    sym_null.m_other = 0;
    sym_null.m_shndx = Elf32SectionHeader::SHN_UNDEF;
    m_buf.insert(m_buf.end(), (uint8_t*)&sym_null, (uint8_t*)&sym_null + sizeof(Elf32Symbol));

    Elf32Symbol sym_sect; //Not sure what the section symbol is for
    sym_sect.m_name = 0;
    sym_sect.m_value = 0;
    sym_sect.m_size = 0;
    sym_sect.m_info = sym_sect.to_info(Elf32Symbol::STB_LOCAL, Elf32Symbol::STT_SECTION);
    sym_sect.m_other = 0;
    sym_sect.m_shndx = Elf32SectionHeader::SHN_UNDEF;
    m_buf.insert(m_buf.end(), (uint8_t*)&sym_sect, (uint8_t*)&sym_sect + sizeof(Elf32Symbol));

    int name_index = 1; //null terminator


    Elf32Symbol sym_file;
    sym_file.m_name = name_index;
    sym_file.m_value = 0;
    sym_file.m_size = 0;
    sym_file.m_info = sym_file.to_info(Elf32Symbol::STB_LOCAL, Elf32Symbol::STT_FILE);
    sym_file.m_other = 0;
    sym_file.m_shndx = Elf32SectionHeader::SHN_ABS;
    m_buf.insert(m_buf.end(), (uint8_t*)&sym_file, (uint8_t*)&sym_file + sizeof(Elf32Symbol));


    name_index += input_file.size() + 1; //includes null-terminator

    for(const std::pair<std::string, Label>& it: m_labels) {
        const Label* l = &it.second;
        Elf32Symbol sym_l;
        sym_l.m_name = name_index;
        sym_l.m_size = 0;
        sym_l.m_info = sym_l.to_info(Elf32Symbol::STB_GLOBAL, Elf32Symbol::STT_NOTYPE);
        sym_l.m_other = 0;
        if (l->m_defined) {
            sym_l.m_value = l->m_addr - ((Elf32SectionHeader*)(m_buf.data() + sh_text_offset))->m_offset;
            sym_l.m_shndx = 1; //text section index TODO: shouldn't hard-code this
        } else {
            sym_l.m_value = 0;
            sym_l.m_shndx = Elf32SectionHeader::SHN_UNDEF;
        }
        m_buf.insert(m_buf.end(), (uint8_t*)&sym_l, (uint8_t*)&sym_l + sizeof(Elf32Symbol));
        name_index += (it.first.size() + 1); //includes null-terminator
    }



    ((Elf32SectionHeader*)(m_buf.data() + sh_symtab_offset))->m_size = m_buf.size() - ((Elf32SectionHeader*)(m_buf.data() + sh_symtab_offset))->m_offset;
}

void Assembler::append_strtab_section(int sh_strtab_offset, const std::string& input_file) {
    align_boundry_to(1);

    ((Elf32SectionHeader*)(m_buf.data() + sh_strtab_offset))->m_offset = m_buf.size();

    m_buf.push_back('\0'); //null string
    m_buf.insert(m_buf.end(), input_file.data(), input_file.data() + input_file.size()); //filename
    m_buf.push_back('\0');
    for (const std::pair<std::string, Label>& it: m_labels) {
        std::string sym = it.first;
        m_buf.insert(m_buf.end(), sym.data(), sym.data() + sym.size());
        m_buf.push_back('\0');
    }

    ((Elf32SectionHeader*)(m_buf.data() + sh_strtab_offset))->m_size = m_buf.size() - ((Elf32SectionHeader*)(m_buf.data() + sh_strtab_offset))->m_offset;
}


void Assembler::append_rel_section(int sh_rel_offset, int sh_text_offset, int sh_symtab_offset, int sh_strtab_offset) {
    align_boundry_to(4);

    ((Elf32SectionHeader*)(m_buf.data() + sh_rel_offset))->m_offset = m_buf.size();

    for (const std::pair<std::string, Label>& it: m_labels) {
        const Label* l = &it.second;
        if (!(l->m_defined)) { //defined symbols have relative jump addresses patched during 'append_program()'
            int sym_count = (((Elf32SectionHeader*)&m_buf[sh_symtab_offset])->m_size) / sizeof(Elf32Symbol);
            int sym_idx = -1;
            for (int i = 0; i < sym_count; i++) {
                Elf32Symbol * sym = (Elf32Symbol*)(m_buf.data() + ((Elf32SectionHeader*)(m_buf.data() + sh_symtab_offset))->m_offset + i * sizeof(Elf32Symbol));
                char* strtab = (char*)(m_buf.data() + ((Elf32SectionHeader*)(m_buf.data() + sh_strtab_offset))->m_offset);
                if (strlen(&strtab[sym->m_name]) == it.first.size() && strncmp(&strtab[sym->m_name], it.first.c_str(), it.first.size()) == 0) {
                    sym_idx = i;
                    break;
                }
            }

            for (uint32_t addr: l->m_rjmp_addr) {
                Elf32Relocation r;
                r.m_offset = addr - ((Elf32SectionHeader*)(m_buf.data() + sh_text_offset))->m_offset;
                r.m_info = r.to_info(sym_idx, Elf32Relocation::R_386_PC32);
                m_buf.insert(m_buf.end(), (uint8_t*)&r, (uint8_t*)&r + sizeof(Elf32Relocation));
            }
        }
    }

    ((Elf32SectionHeader*)(m_buf.data() + sh_rel_offset))->m_size = m_buf.size() - ((Elf32SectionHeader*)(m_buf.data() + sh_rel_offset))->m_offset;
}

void Assembler::generate_obj(const std::string& input_file, const std::string& output_file) {
    read(input_file);
    lex();
    if (ems.count > 0) return;
    parse();
    if (ems.count > 0) return;

    Elf32ElfHeader eh;
    eh.m_shstrndx = 2;
    eh.m_shentsize = sizeof(Elf32SectionHeader);
    m_buf.insert(m_buf.end(), (uint8_t*)&eh, (uint8_t*)&eh + sizeof(Elf32ElfHeader));
    ((Elf32ElfHeader*)(m_buf.data()))->m_ehsize = m_buf.size();
    ((Elf32ElfHeader*)(m_buf.data()))->m_shoff = m_buf.size();


    append_section_header({0, Elf32SectionHeader::SHT_NULL,
                            0, 0, 0, 0,
                            Elf32SectionHeader::SHN_UNDEF, 0, 0, 0});

    int sh_text_offset = append_section_header({1, Elf32SectionHeader::SHT_PROGBITS,
                                                Elf32SectionHeader::SHF_ALLOC | Elf32SectionHeader::SHF_EXECINSTR, 0, 0, 0,
                                                Elf32SectionHeader::SHN_UNDEF, 0, 16, 0});

    int sh_shstrtab_offset = append_section_header({7, Elf32SectionHeader::SHT_STRTAB,
                                                    0, 0, 0, 0,
                                                    Elf32SectionHeader::SHN_UNDEF, 0, 1, 0});

    int sh_symtab_offset = append_section_header({17, Elf32SectionHeader::SHT_SYMTAB,
                                                  0, 0, 0, 0,
                                                  4, 3, 4, sizeof(Elf32Symbol)});

    int sh_strtab_offset = append_section_header({25, Elf32SectionHeader::SHT_STRTAB,
                                                  0, 0, 0, 0,
                                                  0, 0, 1, 0});

    int sh_rel_offset = append_section_header({33, Elf32SectionHeader::SHT_REL,
                                               0, 0, 0, 0,
                                               3, 1, 4, sizeof(Elf32Relocation)});


    bool undefined_globals = false;
    append_text_section(sh_text_offset, &undefined_globals);

    append_shstrtab_section(sh_shstrtab_offset);

    append_symtab_section(sh_symtab_offset, sh_text_offset, input_file);

    append_strtab_section(sh_strtab_offset, input_file);

    if (undefined_globals) {
        append_rel_section(sh_rel_offset, sh_text_offset, sh_symtab_offset, sh_strtab_offset);
    }

    if (ems.count > 0) return;
    write(output_file);


}

int Assembler::append_section_header(Elf32SectionHeader h) {
    int offset = m_buf.size();
    m_buf.insert(m_buf.end(), (uint8_t*)&h, (uint8_t*)&h + sizeof(Elf32SectionHeader));
    return offset;
}


void Assembler::read(const std::string& input_file) {
    std::ifstream f(input_file);
    std::stringstream buffer;
    buffer << f.rdbuf();
    m_assembly = buffer.str();
}

void Assembler::lex() {
    m_tokens = m_lexer.lex(m_assembly, m_reserved_words);
 
    /* 
    for (struct Token t: m_tokens) {
        printf("%.*s\n", t.len, t.start);
    }*/
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
            ems_add(&ems, next.line, "Parse Error: Unrecognized token in memory access!");
            return NULL;
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
    //    std::cout << "instruction: " << n->to_string() << '\n';
    }
}

struct Token Assembler::peek_one() {
    uint32_t next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Assembler::peek_two() {
    uint32_t next = m_current + 1;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Assembler::next_token() {
    uint32_t next = m_current;
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


void Assembler::append_program() {
    for (Node *n: m_nodes) {
        n->assemble(*this);
    }

    patch_rel_jumps();
}


void Assembler::patch_rel_jumps() {
    for(const std::pair<std::string, Label>& it: m_labels) {
        const Label* l = &it.second;
        if (l->m_defined) { //undefined symbols must be resolved by linker
            for (uint32_t addr: l->m_rjmp_addr) {
                uint32_t *ptr = (uint32_t*)&m_buf[addr];
                uint32_t final_addr = l->m_addr - *ptr;
                *ptr = final_addr;
            }
        }
    }
}


void Assembler::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}
