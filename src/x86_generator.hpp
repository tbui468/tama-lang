#ifndef X86_GENERATOR_HPP
#define X86_GENERATOR_HPP

#include <string>
#include <unordered_map>
#include "tac.hpp"
#include "x86_frame.hpp"

class X86Generator {
    public:
        std::vector<uint8_t> m_buf;
    public:
        void write_op(const char* format, ...);
        void generate_asm(const std::vector<TacQuad>* quads, const std::vector<std::string>* labels, const std::unordered_map<std::string, X86Frame>* frames, const std::string& output_file);
        void write(const std::string& output_file);
};

#endif //X86_GENERATOR_HPP
