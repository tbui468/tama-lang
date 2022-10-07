#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

#include "token.hpp"
#include "reserved_word.hpp"

class LexerNew {
    private:
        std::string m_code;
        int m_line;
        int m_current;
        std::vector<struct ReservedWordNew> m_reserved_words;
        std::vector<struct Token> m_tokens;
    public:
        std::vector<struct Token> lex(const std::string& code, const std::vector<struct ReservedWordNew>& reserved_words);
    private:
        void skip_ws();
        bool is_digit(char c);
        bool is_char(char c);
        struct Token read_word();
        struct Token read_number();
        struct Token next_token();
};


#endif //LEXER_HPP
