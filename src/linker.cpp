
#include <fstream>
#include <iterator>
#include <cstring>
#include <iostream>
#include <cassert>

#include "linker.hpp"
#include "elf.hpp"

std::vector<uint8_t> Linker::read_binary(const std::string& input_file) {
    std::ifstream f(input_file, std::ios::binary);
    f.unsetf(std::ios::skipws); //skip newlines

    //get filesize
    std::streampos fsize;
    f.seekg(0, std::ios::end);
    fsize = f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> v;
    v.reserve(fsize);

    v.insert(v.begin(), std::istream_iterator<uint8_t>(f), std::istream_iterator<uint8_t>());
    return v;
}


void Linker::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}


Elf32SectionHeader* Linker::get_section_header(const std::vector<uint8_t>& elf_buf, const std::string& name) {
    Elf32ElfHeader *eh = (Elf32ElfHeader*)elf_buf.data();
    Elf32SectionHeader *shstrtab_sh = (Elf32SectionHeader*)(elf_buf.data() + eh->m_shoff + eh->m_shstrndx * sizeof(Elf32SectionHeader));
    for (int i = 0; i < eh->m_shnum; i++) {
        Elf32SectionHeader* sh = (Elf32SectionHeader*)(elf_buf.data() + eh->m_shoff + i * sizeof(Elf32SectionHeader));
        char* sh_name = (char*)(elf_buf.data() + shstrtab_sh->m_offset + sh->m_name);
        if (strlen(sh_name) == name.size() && strncmp(sh_name, name.c_str(), name.size()) == 0) {
            return sh;
        }
    }

    return nullptr;
}

Elf32Symbol* Linker::get_symbol(const std::vector<uint8_t>& elf_buf, char* name) {
    Elf32SectionHeader* symtab_sh = get_section_header(elf_buf, ".symtab");    
    Elf32SectionHeader* strtab_sh = get_section_header(elf_buf, ".strtab");

    for (int i = 0; i < symtab_sh->m_size / symtab_sh->m_entsize; i++) {
        Elf32Symbol* sym = (Elf32Symbol*)(elf_buf.data() + symtab_sh->m_offset) + i;
        char* sym_name = (char*)&elf_buf[strtab_sh->m_offset + sym->m_name];
        if (strlen(name) == strlen(sym_name) && strncmp(name, sym_name, strlen(name)) == 0) {
            return sym;
        }
    }

    return nullptr;
}

void Linker::link(const std::vector<std::string>& obj_files, const std::string& output_file) {
    for (std::string s: obj_files) {
        m_obj_bufs.push_back(read_binary(s));
    }

    Elf32ElfHeader eh;
    eh.m_type = 2; //executable    
    eh.m_ehsize = sizeof(Elf32ElfHeader);
    eh.m_phoff = sizeof(Elf32ElfHeader);
    eh.m_phentsize = sizeof(Elf32ProgramHeader);
    eh.m_phnum = 1;
    m_buf.insert(m_buf.end(), (uint8_t*)&eh, (uint8_t*)&eh + sizeof(Elf32ElfHeader));

    Elf32ProgramHeader ph;
    ph.m_type = 1;
    ph.m_offset = 0;
    ph.m_vaddr = 0x08048000;
    ph.m_paddr = 0x08048000;
    ph.m_flags = 5;
    ph.m_align = 0x1000;

    int ph_offset = m_buf.size();
    m_buf.insert(m_buf.end(), (uint8_t*)&ph, (uint8_t*)&ph + sizeof(Elf32ProgramHeader));
    int program_offset = m_buf.size();


    //append all text sections
    std::vector<int> module_offsets = std::vector<int>();
    for (const std::vector<uint8_t>& rel_buf: m_obj_bufs) {
        module_offsets.push_back(m_buf.size());

        Elf32SectionHeader* text_sh = get_section_header(rel_buf, ".text");
        assert(text_sh && "Assertion Failed: text section header not found.");

        m_buf.insert(m_buf.end(), rel_buf.data() + text_sh->m_offset, rel_buf.data() + text_sh->m_offset + text_sh->m_size);
    }

    ((Elf32ProgramHeader*)(m_buf.data() + ph_offset))->m_memsz = m_buf.size();
    ((Elf32ProgramHeader*)(m_buf.data() + ph_offset))->m_filesz = m_buf.size();


    int main_module_offset = 0;

    for (int i = 0; i < m_obj_bufs.size(); i++) {
        const std::vector<uint8_t>* rel_buf = &m_obj_bufs.at(i);

        Elf32SectionHeader *strtab_sh = get_section_header(*rel_buf, ".strtab");
        Elf32SectionHeader *symtab_sh = get_section_header(*rel_buf, ".symtab");

        assert(strtab_sh && "Assertion Failed: strtab section header not found.");
        assert(symtab_sh && "Assertion Failed: symtab section header not found.");

        for (int j = 0; j < symtab_sh->m_size / symtab_sh->m_entsize; j++) {
            Elf32Symbol* sym = (Elf32Symbol*)(rel_buf->data() + symtab_sh->m_offset + j * sizeof(Elf32Symbol));
            char* sym_name = (char*)(rel_buf->data() + strtab_sh->m_offset + sym->m_name);
            if (strlen(sym_name) == 6 && strncmp(sym_name, "__main", 6) == 0) {
                ((Elf32ElfHeader*)m_buf.data())->m_entry = program_offset + main_module_offset + sym->m_value + 0x08048000;
            }
        }
        
        main_module_offset += rel_buf->size();
    }

    //apply relocations to each module
    
    int total_offset = 0;
    for (const std::vector<uint8_t>& rel_buf: m_obj_bufs) {
        Elf32SectionHeader *rel_sh = get_section_header(rel_buf, ".rel.text");
        if (!rel_sh) {
            total_offset += get_section_header(rel_buf, ".text")->m_size;
            continue;
        }

        Elf32SectionHeader *symtab_sh = get_section_header(rel_buf, ".symtab");
        Elf32SectionHeader *strtab_sh = get_section_header(rel_buf, ".strtab");

        for (int i = 0; i < rel_sh->m_size / rel_sh->m_entsize; i++) {
            Elf32Relocation *rel = (Elf32Relocation*)(rel_buf.data() + rel_sh->m_offset + i * sizeof(Elf32Relocation));
            Elf32Symbol *sym = (Elf32Symbol*)(rel_buf.data() + symtab_sh->m_offset + rel->get_sym_idx() * sizeof(Elf32Symbol));
            char* sym_name = (char*)(rel_buf.data() + strtab_sh->m_offset + sym->m_name);

            int other_offset = 0;
            for (const std::vector<uint8_t>& other_buf: m_obj_bufs) {
                if (&other_buf == &rel_buf) {
                    other_offset += get_section_header(other_buf, ".text")->m_size;
                    continue;
                }

                Elf32Symbol* other_sym = get_symbol(other_buf, sym_name);
                if (!other_sym) {
                    other_offset += get_section_header(other_buf, ".text")->m_size;
                    continue;
                }

                if (other_sym->m_shndx == Elf32SectionHeader::SHN_UNDEF) {
                    other_offset += get_section_header(other_buf, ".text")->m_size;
                    continue;
                }

                Elf32SectionHeader *other_text_sh = get_section_header(other_buf, ".text");

                uint32_t* final_addr = (uint32_t*)(&other_buf[other_text_sh->m_offset + other_sym->m_value]);
                std::cout << "test: " << other_offset + other_sym->m_value << std::endl;
                std::cout << "program relocation offset: " << program_offset + total_offset + rel->m_offset << std::endl;
                std::cout << "total offset: " << total_offset << std::endl;
                std::cout << "rel->m_offset: " << rel->m_offset << std::endl;

                if (rel->get_type() == Elf32Relocation::R_386_PC32 ) {
                    //Note: relative jumps are based of instruction AFTER current, so we need to relocate based off the instruction after the the address to patch
                    *((uint32_t*)&m_buf[program_offset + total_offset + rel->m_offset]) = other_offset + other_sym->m_value - (total_offset + rel->m_offset + 4);
                }


                other_offset += get_section_header(other_buf, ".text")->m_size;
            }

        }

        total_offset += get_section_header(rel_buf, ".text")->m_size;
    }

    write(output_file);
}
