
#include <fstream>
#include <iterator>
#include <cstring>
#include <iostream>

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
    //ph.m_offset = sizeof(Elf32ElfHeader) + sizeof(Elf32ProgramHeader);
    ph.m_offset = 0;
    ph.m_vaddr = 0x08048000;
    ph.m_paddr = 0x08048000;
    ph.m_flags = 5;
    ph.m_align = 0x1000;

    int ph_offset = m_buf.size();
    m_buf.insert(m_buf.end(), (uint8_t*)&ph, (uint8_t*)&ph + sizeof(Elf32ProgramHeader));
    int program_offset = m_buf.size();


    std::vector<int> module_offsets = std::vector<int>();
    for (const std::vector<uint8_t>& rel_buf: m_obj_bufs) {
        module_offsets.push_back(m_buf.size());

        Elf32ElfHeader* rel_eh = (Elf32ElfHeader*)rel_buf.data();
        Elf32SectionHeader* shstrtab_sh = (Elf32SectionHeader*)(rel_buf.data() + rel_eh->m_shoff + rel_eh->m_shstrndx * sizeof(Elf32SectionHeader));
        Elf32SectionHeader* text_sh = nullptr;
        for (int i = 0; i < rel_eh->m_shnum; i++) {
            Elf32SectionHeader* rel_sh = (Elf32SectionHeader*)(rel_buf.data() + rel_eh->m_shoff + i * sizeof(Elf32SectionHeader));
            char* name = (char*)(rel_buf.data() + shstrtab_sh->m_offset + rel_sh->m_name);
            if (strncmp(name, ".text", 5) == 0) {
                text_sh = rel_sh;
                break;
            }
        }

        int text_offset = text_sh->m_offset;
        int text_size = text_sh->m_size;
        m_buf.insert(m_buf.end(), rel_buf.data() + text_offset, rel_buf.data() + text_offset + text_size);
    }

    ((Elf32ProgramHeader*)(m_buf.data() + ph_offset))->m_memsz = m_buf.size();
    ((Elf32ProgramHeader*)(m_buf.data() + ph_offset))->m_filesz = m_buf.size();


    int main_module_offset = 0;
    int main_offset = 0;

    for (int i = 0; i < m_obj_bufs.size(); i++) {
        std::vector<uint8_t>* rel_buf = &m_obj_bufs.at(i);
        Elf32ElfHeader *rel_eh = (Elf32ElfHeader*)(rel_buf->data());
        Elf32SectionHeader* shstrtab_sh = (Elf32SectionHeader*)(rel_buf->data() + rel_eh->m_shoff + rel_eh->m_shstrndx * sizeof(Elf32SectionHeader));
        Elf32SectionHeader* strtab_sh = nullptr;
        Elf32SectionHeader* symtab_sh = nullptr;

        for (int j = 0; j < rel_eh->m_shnum; j++) {
            Elf32SectionHeader* rel_sh = (Elf32SectionHeader*)(rel_buf->data() + rel_eh->m_shoff + j * sizeof(Elf32SectionHeader));
            char* rel_sh_name = (char*)(rel_buf->data() + shstrtab_sh->m_offset + rel_sh->m_name);
            if (strncmp(rel_sh_name, ".strtab", 7) == 0) {
                strtab_sh = rel_sh;
            } else if (strncmp(rel_sh_name, ".symtab", 7) == 0) {
                symtab_sh = rel_sh;
            }
        }

        //at this point strtab_sh and symtab_sh should not be null
        if (!strtab_sh || !symtab_sh) {
            printf("Something broke in Linker::link\n");
            exit(1);
        }

        
        for (int j = 0; j < symtab_sh->m_size / symtab_sh->m_entsize; j++) {
            Elf32Symbol* sym = (Elf32Symbol*)(rel_buf->data() + symtab_sh->m_offset + j * sizeof(Elf32Symbol));
            char* sym_name = (char*)(rel_buf->data() + strtab_sh->m_offset + sym->m_name);
            if (strlen(sym_name) == 6 && strncmp(sym_name, "__main", 6) == 0) {
                main_offset = sym->m_value;
                ((Elf32ElfHeader*)m_buf.data())->m_entry = program_offset + main_module_offset + main_offset + 0x08048000;
            }
        }

        
        main_module_offset += rel_buf->size();
    }

/*
    std::cout << "Program offset: " << program_offset << std::endl;
    std::cout << "main module offset: " << main_module_offset << std::endl;
    std::cout << "main offset: " << main_offset << std::endl;
    ((Elf32ElfHeader*)m_buf.data())->m_entry = program_offset + main_module_offset + main_offset + 0x08048000;*/
    
    //TODO: apply relocations
    //  symbol relocations using relocation entries
    //      for each relocation entry
    //          extract symbol index from rel.m_info
    //          final_offset = symbol offset + module_offset
    //          use rel.m_offset + module_offset to find location to patch - patch it with final_offset
    //
    //  add load offset (the 0x08048000)


    write(output_file);
}
