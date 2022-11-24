#include <fstream>
#include <iterator>
#include <cstring>
#include <iostream>
#include <cassert>

#include "linker.hpp"
#include "elf.hpp"
#include "error.hpp"

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


void Linker::write_elf_executable(const std::string& output_file) {
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

    for (int i = 0; i < int(symtab_sh->m_size / symtab_sh->m_entsize); i++) {
        Elf32Symbol* sym = (Elf32Symbol*)(elf_buf.data() + symtab_sh->m_offset) + i;
        char* sym_name = (char*)&elf_buf[strtab_sh->m_offset + sym->m_name];
        if (strlen(name) == strlen(sym_name) && strncmp(name, sym_name, strlen(name)) == 0) {
            return sym;
        }
    }

    return nullptr;
}


void Linker::read_elf_relocatables(const std::vector<std::string>& obj_files) {
    for (std::string s: obj_files) {
        m_obj_bufs.insert({s, read_binary(s)});
    }
}


void Linker::append_elf_executable_header() {
    Elf32ElfHeader eh;
    eh.m_type = 2; //executable    
    eh.m_ehsize = sizeof(Elf32ElfHeader);
    eh.m_phoff = sizeof(Elf32ElfHeader);
    eh.m_phentsize = sizeof(Elf32ProgramHeader);
    eh.m_phnum = 1;
    m_buf.insert(m_buf.end(), (uint8_t*)&eh, (uint8_t*)&eh + sizeof(Elf32ElfHeader));
}


void Linker::append_program_header() {
    Elf32ProgramHeader ph;
    ph.m_type = 1;
    ph.m_offset = 0; //TODO: Understnd why is this 0 and not the program segment offset? It crashes if set to that...
    ph.m_vaddr = Linker::LOAD_ADDR;
    ph.m_paddr = Linker::LOAD_ADDR;
    ph.m_flags = 5;
    ph.m_align = 0x1000; //TODO: Need to figure out what this is for
    m_buf.insert(m_buf.end(), (uint8_t*)&ph, (uint8_t*)&ph + sizeof(Elf32ProgramHeader));
}

void Linker::append_program() {
    for (const std::pair<std::string, std::vector<uint8_t>>& p: m_obj_bufs) {

        m_code_offsets.insert({p.first, m_buf.size() - sizeof(Elf32ElfHeader) - sizeof(Elf32ProgramHeader)});

        Elf32SectionHeader* text_sh = get_section_header(p.second, ".text");
        assert(text_sh && "Assertion Failed: text section header not found.");

        m_buf.insert(m_buf.end(), p.second.data() + text_sh->m_offset, p.second.data() + text_sh->m_offset + text_sh->m_size);
    }

    int ph_offset = ((Elf32ElfHeader*)m_buf.data())->m_phoff;

    ((Elf32ProgramHeader*)(m_buf.data() + ph_offset))->m_memsz = m_buf.size();
    ((Elf32ProgramHeader*)(m_buf.data() + ph_offset))->m_filesz = m_buf.size();
}


void Linker::patch_program_entry() {
    int program_offset = sizeof(Elf32ElfHeader) + sizeof(Elf32ProgramHeader);

    for (const std::pair<std::string, std::vector<uint8_t>>& p: m_obj_bufs) {
        const std::vector<uint8_t>* rel_buf = &p.second;
        int code_offset = m_code_offsets.find(p.first)->second;

        Elf32SectionHeader *strtab_sh = get_section_header(*rel_buf, ".strtab");
        Elf32SectionHeader *symtab_sh = get_section_header(*rel_buf, ".symtab");

        assert(strtab_sh && "Assertion Failed: strtab section header not found.");
        assert(symtab_sh && "Assertion Failed: symtab section header not found.");

        for (int j = 0; j < int(symtab_sh->m_size / symtab_sh->m_entsize); j++) {
            Elf32Symbol* sym = (Elf32Symbol*)(rel_buf->data() + symtab_sh->m_offset + j * sizeof(Elf32Symbol));
            char* sym_name = (char*)(rel_buf->data() + strtab_sh->m_offset + sym->m_name);
            const char* entry_name = "_start";
            if (strlen(sym_name) == strlen(entry_name) && strncmp(sym_name, entry_name, strlen(entry_name)) == 0) {
                ((Elf32ElfHeader*)m_buf.data())->m_entry = program_offset + code_offset + sym->m_value + Linker::LOAD_ADDR;
            }
        }
    }
}

void Linker::apply_relocations() {

    for (const std::pair<std::string, std::vector<uint8_t>>& p: m_obj_bufs) {
        const std::vector<uint8_t> rel_buf = p.second;
        int rel_code_offset = m_code_offsets.find(p.first)->second;
        Elf32SectionHeader *rel_sh = get_section_header(rel_buf, ".rel.text");

        if (!rel_sh) continue;

        Elf32SectionHeader *symtab_sh = get_section_header(rel_buf, ".symtab");
        Elf32SectionHeader *strtab_sh = get_section_header(rel_buf, ".strtab");

        for (int i = 0; i < int(rel_sh->m_size / rel_sh->m_entsize); i++) {
            Elf32Relocation *rel = (Elf32Relocation*)(rel_buf.data() + rel_sh->m_offset + i * sizeof(Elf32Relocation));
            Elf32Symbol *sym = (Elf32Symbol*)(rel_buf.data() + symtab_sh->m_offset + rel->get_sym_idx() * sizeof(Elf32Symbol));
            char* sym_name = (char*)(rel_buf.data() + strtab_sh->m_offset + sym->m_name);

            bool found_def = false;
            for (const std::pair<std::string, std::vector<uint8_t>>& p: m_obj_bufs) {
                const std::vector<uint8_t> other_buf = p.second;
                int defined_sym_code_offset = m_code_offsets.find(p.first)->second;

                Elf32Symbol* defined_sym = get_symbol(other_buf, sym_name);
                if (&other_buf != &rel_buf && defined_sym && defined_sym->m_shndx != Elf32SectionHeader::SHN_UNDEF) {
                    found_def = true;

                    if (rel->get_type() == Elf32Relocation::R_386_PC32 ) {
                        //Note: relative jumps are based of instruction AFTER current, so we need to relocate based off the instruction after the the address to patch
                        int addr_to_patch = sizeof(Elf32ElfHeader) + sizeof(Elf32ProgramHeader) + rel_code_offset + rel->m_offset;
                        *((uint32_t*)&m_buf[addr_to_patch]) = defined_sym_code_offset + defined_sym->m_value - (rel_code_offset + rel->m_offset + 4);
                    } else {
                        assert(false && "Assertion Failed: Linker only supports R_386_PC32 relocation types for now.");
                    }

                }
            }

            if (!found_def) {
                ems.add_error(0, "Linker Error: Symbol '%s' not defined in any translation units.", sym_name);
            }

        }

    }
}

void Linker::link(const std::vector<std::string>& obj_files, const std::string& output_file) {
    read_elf_relocatables(obj_files);
    append_elf_executable_header();
    append_program_header();
    append_program();
    patch_program_entry();
    apply_relocations();
    write_elf_executable(output_file);
}
