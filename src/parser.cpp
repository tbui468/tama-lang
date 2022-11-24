#include "parser.hpp"
#include "error.hpp"

struct Token Parser::peek_one() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Parser::peek_two() {
    int next = m_current + 1;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token Parser::next_token() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    struct Token ret = m_tokens[next];
    m_current++;
    return ret;
}

struct Token Parser::consume_token(enum TokenType tt) {
    struct Token t = next_token();
    if (t.type != tt) {
        ems.add_error(t.line, "Unexpected token!");
    }
    return t;
}

bool Parser::end_of_tokens() {
    return peek_one().type == T_EOF;
}
