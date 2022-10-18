#ifndef SEMANT_HPP
#define SEMANT_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <stdarg.h>

#include "reserved_word.hpp"
#include "token.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "parser_new.hpp"
#include "error.hpp"
#include "environment.hpp"

class Semant {

    class TmdParser: public ParserNew {
        public:

        public:
            std::vector<Ast*> parse_tokens(const std::vector<struct Token>& tokens) override {
                m_tokens = tokens;
                m_current = 0;

                while (!end_of_tokens()) {
                    m_nodes.push_back(parse_stmt());
                }
                return m_nodes;
            }
        private:
            Ast *parse_group();
            Ast *parse_literal();
            Ast *parse_unary();
            Ast *parse_factor();
            Ast *parse_term();
            Ast *parse_inequality();
            Ast *parse_equality();
            Ast *parse_and();
            Ast *parse_or();
            Ast *parse_assignment();
            Ast *parse_expr();
            Ast *parse_block();
            Ast *parse_stmt();
    };

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
        TmdParser m_parser;
        std::vector<uint8_t> m_buf;
        Environment m_env;
    public:
        void generate_asm(const std::string& input_file, const std::string& output_file);
        void write_op(const char* format, ...);
    private: 
        void read(const std::string& input_file);
        void lex();
        void parse();
        void translate();
        void write(const std::string& output_file);
};

#endif //SEMANT_HPP
