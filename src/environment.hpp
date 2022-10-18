#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

class Symbol {
    public:
        std::string m_symbol;
        enum TokenType m_dtype;
        int m_bp_offset;
    public:
        Symbol(struct Token symbol, struct Token type, int bp_offset): 
            m_symbol(std::string(symbol.start, symbol.len)), m_dtype(type.type), m_bp_offset(bp_offset) {}
};

class Environment {
    private:
        class Scope {
            public:
                Scope* m_next;
                std::unordered_map<std::string, Symbol> m_symbols;
            public:
                Scope(Scope* next): m_next(next) {}
        };
    public:
        Scope* m_head = nullptr;
    public:
        Symbol* get_symbol(struct Token symbol) {
            Scope* current = m_head;
            while(current) {
                std::unordered_map<std::string, Symbol>::iterator it = current->m_symbols.find(std::string(symbol.start, symbol.len));
                if (it != current->m_symbols.end()) {
                    return &(it->second);
                }
                current = current->m_next;
            }

            return nullptr;
        }
        bool declared_in_scope(struct Token symbol) {
            if (m_head) {
                std::unordered_map<std::string, Symbol>::iterator it = m_head->m_symbols.find(std::string(symbol.start, symbol.len));
                if (it != m_head->m_symbols.end()) {
                    return true;
                }
            }

            return false;
        }
        bool add_symbol(struct Token symbol, struct Token type) {
            if (declared_in_scope(symbol)) {
                return false; 
            }

            int offset = 0;
            Scope* current = m_head;
            while (current) {
                offset += current->m_symbols.size();
                current = current->m_next;
            }

            m_head->m_symbols.insert({std::string(symbol.start, symbol.len), Symbol(symbol, type, offset)});
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
