#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include "ast.hpp"
#include "token.hpp"

class Parser {
    public:
        std::vector<struct Token> m_tokens;
        int m_current;
        std::vector<Ast*> m_nodes;

    public:
        virtual std::vector<Ast*> parse_tokens(const std::vector<struct Token>& tokens) = 0;
    protected:
        struct Token peek_two();
        struct Token peek_one();
        struct Token next_token();
        struct Token consume_token(enum TokenType tt);
        bool end_of_tokens();
};


#endif //PARSER_HPP
