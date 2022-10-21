#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP


class Type {
    public:
        //if m_dtype == T_FUN_TYPE, then m_rtype and m_params can be used to find function type
        enum TokenType m_dtype;
        enum TokenType m_rtype;
        std::vector<Type> m_ptypes;
    public:
        Type(enum TokenType dtype, enum TokenType rtype, std::vector<Type> params):
            m_dtype(dtype), m_rtype(rtype), m_ptypes(params) {}
        Type(enum TokenType dtype): m_dtype(dtype), m_rtype(T_NIL_TYPE), m_ptypes(std::vector<Type>()) {}
        bool is_of_type(const Type& other) {
            if (m_dtype != other.m_dtype) return false;
            if (m_ptypes.size() != other.m_ptypes.size()) return false;

            if (m_dtype == T_FUN_TYPE) {
                for (int i = 0; i < m_ptypes.size(); i++) {
                    if (!m_ptypes.at(i).is_of_type(other.m_ptypes.at(i))) return false;
                }
            }

            return true;
        }
};

class Symbol {
    public:
        std::string m_symbol;
        Type m_type;
        int m_bp_offset;
    public:
        Symbol(struct Token symbol, Type type, int bp_offset):
            m_symbol(std::string(symbol.start, symbol.len)), m_type(type), m_bp_offset(bp_offset) {}
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

        bool add_symbol(struct Token symbol, Type type) {
            if (get_symbol(symbol)) {
                return false; 
            }

            int offset = 0;
            m_symbols.insert({std::string(symbol.start, symbol.len), Symbol(symbol, type, offset)});

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
        bool add_symbol(struct Token symbol, Type type) {
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
