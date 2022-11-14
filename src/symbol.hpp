#ifndef SYMBOL_HPP
#define SYMBOL_HPP

class Symbol {
    public:
        std::string m_name;
        std::string m_tac_name;
        Type m_type;
        int m_fp_offset;
    public:
        Symbol(const std::string& name, const std::string& tac_name, Type type, int fp_offset):
            m_name(name), m_tac_name(tac_name), m_type(type), m_fp_offset(fp_offset) {}
};


class Scope {
    public:
        std::unordered_map<std::string, Symbol> m_symbols;
    public:
        Symbol* get_symbol(const std::string& name) {
            std::unordered_map<std::string, Symbol>::iterator it = m_symbols.find(name);
            if (it != m_symbols.end()) {
                return &(it->second);
            }

            return nullptr;
        }
};

#endif //SYMBOL_HPP
