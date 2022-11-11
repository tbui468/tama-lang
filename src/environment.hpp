#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include "type.hpp"

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
        Scope* m_next;
        std::unordered_map<std::string, Symbol> m_symbols;
    public:
        Scope(Scope* next): m_next(next) {}
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

class Environment {
    public:
        Scope* m_head = nullptr;
    public:
        //TODO: refactor to get_local()
        Symbol* get_symbol(struct Token symbol) {
            Scope* current = m_head;
            while(current) {
                //TODO: refactor to use Scope::get_symbol
                std::unordered_map<std::string, Symbol>::iterator it = current->m_symbols.find(std::string(symbol.start, symbol.len));
                if (it != current->m_symbols.end()) {
                    return &(it->second);
                }
                current = current->m_next;
            }

            return nullptr;
        }
        //TODO: refactor to local_count
        int symbol_count() {
            int count = 0;
            Scope* current = m_head;
            while(current) {
                count += current->m_symbols.size();
                current = current->m_next;
            }

            return count;
        }
        //TODO: refactor to local_decl_in_current_scope
        bool declared_in_scope(struct Token symbol) {
            if (m_head) {
                std::unordered_map<std::string, Symbol>::iterator it = m_head->m_symbols.find(std::string(symbol.start, symbol.len));
                if (it != m_head->m_symbols.end()) {
                    return true;
                }
            }

            return false;
        }
        //TODO: refactor to add_local
        bool add_symbol(struct Token symbol, const std::string& tac_symbol, Type type) {
            if (declared_in_scope(symbol)) {
                return false; 
            }

            int offset = 0;
            Scope* current = m_head;
            while (current) {
                offset += current->m_symbols.size();
                current = current->m_next;
            }

            m_head->m_symbols.insert({std::string(symbol.start, symbol.len), Symbol(symbol, tac_symbol, type, offset)});
            return true;
        }
        void begin_scope() {
            Scope* s = new Scope(m_head);
            m_head = s;
        }

        int end_scope() {
            Scope* old = m_head;
            m_head = old->m_next;
            int count = old->m_symbols.size();
            delete old;
            return count;
        }
};

#endif //ENVIRONMENT_HPP
