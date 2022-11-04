#ifndef LINKER_HPP
#define LINKER_HPP

#include <vector>
#include <string>

#include "elf.hpp"

class Linker {
    private:
        std::vector<uint8_t> m_buf;
        std::vector<std::vector<uint8_t>> m_obj_bufs;
    private:
        std::vector<uint8_t> read_binary(const std::string& input_file);
        void write(const std::string& output_file);
        Elf32SectionHeader* get_section_header(const std::vector<uint8_t>& elf_buf, const std::string& name);
        Elf32Symbol* get_symbol(const std::vector<uint8_t>& elf_buf, char* name);
    public:
        void link(const std::vector<std::string>& input_files, const std::string& output_file);
};


#endif //LINKER_HPP
