#ifndef LINKER_HPP
#define LINKER_HPP

#include <vector>
#include <unordered_map>
#include <string>

#include "elf.hpp"

class Linker {
    private:
        std::vector<uint8_t> m_buf;
        //std::vector<std::vector<uint8_t>> m_obj_bufs;
        std::unordered_map<std::string, std::vector<uint8_t>> m_obj_bufs;
        std::unordered_map<std::string, int> m_code_offsets;
        static const uint32_t LOAD_ADDR = 0x08048000;
    private:
        std::vector<uint8_t> read_binary(const std::string& input_file);
        void write_elf_executable(const std::string& output_file);
        Elf32SectionHeader* get_section_header(const std::vector<uint8_t>& elf_buf, const std::string& name);
        Elf32Symbol* get_symbol(const std::vector<uint8_t>& elf_buf, char* name);
        void read_elf_relocatables(const std::vector<std::string>& obj_files);
        void append_elf_executable_header();
        void append_program_header();
        void append_program();
        void patch_program_entry();
        void apply_relocations();
    public:
        void link(const std::vector<std::string>& input_files, const std::string& output_file);
};


#endif //LINKER_HPP
