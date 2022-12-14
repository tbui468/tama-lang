#include <iostream>
#include <cassert>

#include "x86_frame.hpp"


int X86Frame::s_temp_counter = 0;

void X86Frame::begin_scope() {
    Scope s;
    m_scopes.push_back(s);
}

int X86Frame::end_scope() {
    Scope s = m_scopes.back();

    for (const std::pair<std::string, Symbol>& p: s.m_symbols) {
        m_symbols.insert({p.second.m_tac_name, p.second});
    }

    m_scopes.pop_back();
    return s.m_symbols.size();
}


std::string X86Frame::add_local(const std::string& reg_name, Type type) {
    assert(!symbol_defined_in_current_scope(reg_name));

    std::string tac_name = "_t" + std::to_string((X86Frame::s_temp_counter++)) + reg_name;

    int offset = 0;
    for (const Scope& s: m_scopes) {
        offset += s.m_symbols.size();
    }


    m_scopes.back().m_symbols.insert({reg_name == "" ? tac_name : reg_name, Symbol(reg_name, tac_name, type, -4 * (offset + 1))});

    return tac_name;
}

std::string X86Frame::add_temp(Type type) {
    return add_local("", type);
}

bool X86Frame::symbol_defined_in_current_scope(const std::string& name) {
    return m_scopes.back().get_symbol(name);
}

bool X86Frame::add_parameter_to_frame(const std::string& name, Type type, int ord_num) {
    std::unordered_map<std::string, Symbol>::iterator it = m_symbols.find(name);
    if (it != m_symbols.end()) {
        return false;
    }

    m_symbols.insert({name, Symbol(name, "", type, 4 * (ord_num + 2))});
    return true;
}

Symbol* X86Frame::get_symbol_from_scopes(const std::string& name) {

    for (int i = m_scopes.size() - 1; i >= 0; i--) {
        std::unordered_map<std::string, Symbol>::iterator it = m_scopes.at(i).m_symbols.find(name);

        if (it == m_scopes.at(i).m_symbols.end()) continue;

        if (it->first == name) {
            return &(it->second);
        }        
    }

    return nullptr;
}

Symbol* X86Frame::get_symbol_from_frame(const std::string& name) {
    std::unordered_map<std::string, Symbol>::iterator it = m_symbols.find(name);
    if (it == m_symbols.end())
        return nullptr;

    return &it->second;
}

int X86Frame::symbol_count_in_scopes() {
    int count = 0;
    for (const Scope& s: m_scopes) {
        count += s.m_symbols.size();
    }

    return count;
}
