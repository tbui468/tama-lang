#include "parser_new.hpp"
#include "error.hpp"

struct Token ParserNew::peek_one() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token ParserNew::peek_two() {
    int next = m_current + 1;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    return m_tokens[next];
}

struct Token ParserNew::next_token() {
    int next = m_current;
    if (next >= m_tokens.size())
        next = m_tokens.size() - 1;
    struct Token ret = m_tokens[next];
    m_current++;
    return ret;
}

struct Token ParserNew::consume_token(enum TokenType tt) {
    struct Token t = next_token();
    if (t.type != tt) {
        ems_add(&ems, t.line, "Unexpected token!");
    }
    return t;
}

bool ParserNew::end_of_tokens() {
    return peek_one().type == T_EOF;
}
