#ifndef X86_FRAME_HPP
#define X86_FRAME_HPP

#include <unordered_map>
#include <string>
#include <stack>

#include "type.hpp"

class X86Frame {
    public:
        class Symbol {
            public:
                std::string m_symbol;
                std::string m_tac_symbol;
                Type m_type;
                int m_bp_offset;
            public:
                Symbol(struct Token symbol, const std::string& tac_symbol, Type type, int bp_offset):
                    m_symbol(std::string(symbol.start, symbol.len)), m_tac_symbol(tac_symbol), m_type(type), m_bp_offset(bp_offset) {}
        };

        class Scope {
            public:
                std::unordered_map<std::string, Symbol> m_symbols;
            public:
                Symbol* get_symbol(struct Token symbol) {
                    std::unordered_map<std::string, Symbol>::iterator it = m_symbols.find(std::string(symbol.start, symbol.len));
                    if (it != m_symbols.end()) {
                        return &(it->second);
                    }

                    return nullptr;
                }

                bool add_symbol(struct Token symbol, const std::string& tac_symbol, Type type) {
                    if (get_symbol(symbol)) {
                        return false; 
                    }

                    int offset = 0;
                    m_symbols.insert({std::string(symbol.start, symbol.len), Symbol(symbol, tac_symbol, type, offset)});

                    return true;
                }
        };

    public:
        std::unordered_map<std::string, int> m_fp_offsets; //filled in during compilation to ir, used during code generation
        std::stack<Scope> m_scopes; //used to track scopes during compilation to ir
    public:
        void insert_local(std::string temp, int offset);
        void begin_scope();
        int end_scope();
        /*
        bool add_symbol(struct Token symbol, const std::string& tac_symbol, Type type);
        Symbol* get_symbol(struct Token symbol);
        bool declared_in_scope(struct Token symbol);
        int symbol_count();*/
};

#endif //X86_FRAME_HPP
