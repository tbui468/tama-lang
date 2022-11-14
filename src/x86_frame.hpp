#ifndef X86_FRAME_HPP
#define X86_FRAME_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include "type.hpp"
#include "symbol.hpp"

class X86Frame {
    public:
        std::unordered_map<std::string, Symbol> m_symbols;
        std::vector<Scope> m_scopes; //used to track scopes during compilation to ir
        static int s_temp_counter;
    public:
        void begin_scope();
        int end_scope();
        Symbol* get_symbol_from_scopes(const std::string& name);
        Symbol* get_symbol_from_frame(const std::string& name);
        //bool add_symbol_to_scope(const std::string& name, const std::string& tac_name, Type type);
        std::string add_local(const std::string& reg_name, Type type);
        std::string add_temp(Type type);
        bool symbol_defined_in_current_scope(const std::string& name);
        bool add_parameter_to_frame(const std::string& name, Type type, int ord_num);
        int symbol_count_in_scopes();
        inline void print_symbols() {
            for (std::pair<std::string, Symbol> p: m_symbols) {
                std::cout << p.first << ", ";
            }
            std::cout << std::endl;
        }
};

#endif //X86_FRAME_HPP
