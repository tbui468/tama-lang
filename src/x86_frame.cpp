#include "x86_frame.hpp"


void X86Frame::insert_local(std::string temp, int offset) {
    m_fp_offsets.insert({temp, -4 * (offset + 1)});
}


void X86Frame::begin_scope() {
    Scope s;
    m_scopes.push(s);
}

int X86Frame::end_scope() {
    Scope s = m_scopes.top();
    m_scopes.pop();
    return s.m_symbols.size();
}
