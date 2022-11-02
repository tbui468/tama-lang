
#include <fstream>
#include <iterator>
#include <cstring>

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

    //TODO: need to patch m_entry to '_start' symbol
    Elf32ElfHeader eh;
    eh.m_type = 2; //executable    
    eh.m_ehsize = sizeof(Elf32ElfHeader);
    eh.m_phoff = sizeof(Elf32ElfHeader);
    eh.m_phentsize = sizeof(Elf32ProgramHeader);
    eh.m_phnum = 1;
    m_buf.insert(m_buf.end(), (uint8_t*)&eh, (uint8_t*)&eh + sizeof(Elf32ElfHeader));

    //TODO: need to patch m_memsz and m_filesz
    Elf32ProgramHeader ph;
    ph.m_type = 1;
    ph.m_offset = 0;
    ph.m_vaddr = 0x08048000;
    ph.m_paddr = 0x08048000;
    ph.m_flags = 5;
    ph.m_align = 0x1000;

    m_buf.insert(m_buf.end(), (uint8_t*)&ph, (uint8_t*)&ph + sizeof(Elf32ProgramHeader));

    //paste text sections into buffer, and also cache offset of each module's text section in buffer
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

    ((Elf32ProgramHeader*)(m_buf.data() + sizeof(Elf32ElfHeader)))->m_memsz = m_buf.size(); //TODO: should use elfheader->m_phoff
    ((Elf32ProgramHeader*)(m_buf.data() + sizeof(Elf32ElfHeader)))->m_filesz = m_buf.size(); //TODO: should use elfheader->m_phoff


    //TODO: patch entry point
    //  search all relocation files for '__main' symbol 
    //  symbol_entry->m_value will be the offset in the text section for that symbol - patch eh->m_entry to this number + module_offset
    
    //TODO: apply relocations
    //  symbol relocations using relocation entries
    //  add load offset (the 0x08048000)


    write(output_file);
}
