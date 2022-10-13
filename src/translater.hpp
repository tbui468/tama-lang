#ifndef TRANSLATER_HPP
#define TRANSLATER_HPP

#include <string>

#include "token.hpp"

class Translater {
    public:
        class Node {

        };
    public:
        void emit_asm(const std::string& input, const std::string& output);
    private:
        /*
        void read(const std::string& input_file);
        void lex();

        void parse();
        Node* parse_expr();
        Node* parse_stmt();

        struct Token peek_one();
        struct Token peek_two();
        struct Token next_token();
        struct Token consume_token(enum TokenType tt);
        bool end_of_tokens();

        void translate();
        void write(const std::string& output_file);*/
};

#endif //TRANSLATER_HPP
