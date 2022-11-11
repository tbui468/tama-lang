#ifndef X86_FRAME_HPP
#define X86_FRAME_HPP

#include <unordered_map>
#include <string>

class X86Frame {
    public:
        std::unordered_map<std::string, int> m_fp_offsets;

    public:
        X86Frame() {}
};

#endif //X86_FRAME_HPP
