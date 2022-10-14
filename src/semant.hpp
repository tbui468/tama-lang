#ifndef SEMANT_HPP
#define SEMANT_HPP

#include <string>
#include <vector>

#include "reserved_word.hpp"
#include "token.hpp"
#include "lexer.hpp"

class Ast {

};

class Semant {
    public:
        inline static std::vector<struct ReservedWordNew> m_reserved_words {{
            {"print", T_PRINT},
            {"int", T_INT_TYPE},
            {"bool", T_BOOL_TYPE},
            {"true", T_TRUE},
            {"false", T_FALSE},
            {"and", T_AND},
            {"or", T_OR},
            {"if", T_IF},
            {"elif", T_ELIF},
            {"else", T_ELSE},
            {"while", T_WHILE}
        }};
    public:
        std::string m_code = "";
        std::vector<struct Token> m_tokens = std::vector<struct Token>();
        int m_current = 0;
        std::vector<Ast*> m_nodes = std::vector<Ast*>();
        Lexer m_lexer;
    public:
        std::vector<Ast*> generate_ast(const std::string& input_file);
    private: 
        void read(const std::string& input_file);
        void lex();
        void parse();
        void type_check();
};

#endif //SEMANT_HPP
