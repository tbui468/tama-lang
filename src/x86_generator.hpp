#ifndef X86_GENERATOR_HPP
#define X86_GENERATOR_HPP

#include <string>
#include <unordered_map>
#include "tac.hpp"
#include "x86_frame.hpp"
#include "ControlFlowGraph.hpp"

class X86Generator {
    public:
        std::vector<uint8_t> m_buf;
        std::string m_frame_name = "";
        std::string m_frame_size = "";
        const std::unordered_map<std::string, X86Frame>* m_frames;
    public:
        int symbol_offset(const std::string& sym_name);
        void write_op(const char* format, ...);
        void generate_asm(const ControlFlowGraph& cfg, const std::vector<TacQuad>* quads, const std::vector<std::string>* labels, const std::unordered_map<std::string, X86Frame>* frames, const std::string& output_file);
        void write(const std::string& output_file);
        void fetch(const std::string& dst, const std::string& src);
        void store(const std::string& dst, const std::string& src);
};

#endif //X86_GENERATOR_HPP
