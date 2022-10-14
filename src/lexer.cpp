#include <cstring>

#include "lexer.hpp"
#include "error.hpp"

std::vector<struct Token> Lexer::lex(const std::string& code, const std::vector<struct ReservedWordNew>& reserved_words) {
    m_code = code;
    m_line = 1;
    m_current = 0;
    m_reserved_words = reserved_words;
    m_tokens = std::vector<struct Token>();

    while (m_current < m_code.size()) {
        struct Token t = next_token();
        if (t.type != T_NEWLINE)
            m_tokens.push_back(t);
        else
            m_line++;
    }

    m_tokens.push_back({T_EOF, NULL, 0, m_line});

    return m_tokens;
}

void Lexer::skip_ws() {
    while (m_code[m_current] == ' ')
        m_current++;
}

bool Lexer::is_digit(char c) {
    return '0' <= c && c <= '9';
}

bool Lexer::is_char(char c) {
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

struct Token Lexer::read_word() {
    //TODO: use {} constructor
    struct Token t;
    t.start = &m_code[m_current];
    t.len = 1;
    t.line = m_line;

    while (1) {
        char next = m_code[m_current + t.len];
        if (!(is_digit(next) || is_char(next) || next == '_'))
            break;
        t.len++;
    }

    t.type = T_IDENTIFIER;
    for (int i = 0; i < m_reserved_words.size(); i++) {
        struct ReservedWordNew *rw = &m_reserved_words[i];
        if (rw->m_string.size() == t.len && strncmp(rw->m_string.c_str(), t.start, t.len) == 0) {
            t.type = rw->m_token_type;
            break;
        } 
    }

    return t;
}

struct Token Lexer::read_number() {
    struct Token t;
    t.start = &m_code[m_current];
    t.len = 1;
    t.line = m_line;
    bool has_decimal = *t.start == '.';
    bool is_hex = false;

    if (m_code[m_current] == '0' && m_code[m_current + 1] == 'x') {
        is_hex = true;
        t.len++;
    }

    while (1) {
        char next = m_code[m_current + t.len];
        if (is_digit(next) || (is_hex && is_char(next))) {
            t.len++;
        } else if (next == '.') {
            t.len++;
            if (has_decimal) {
                ems_add(&ems, m_line, "Token Error: Too many decimals!");
            } else {
                has_decimal = true;
            }
        } else {
            break;
        }
    }

    t.type = has_decimal ? T_FLOAT : is_hex ? T_HEX : T_INT;
    return t;
}

struct Token Lexer::next_token() {
    skip_ws();

    struct Token t;
    t.line = m_line;
    char c = m_code[m_current];
    switch (c) {
        case '\n':
            t.type = T_NEWLINE;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '+':
            t.type = T_PLUS;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '-':
            t.type = T_MINUS;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '*':
            t.type = T_STAR;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '/':
            t.type = T_SLASH;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '(':
            t.type = T_L_PAREN;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case ')':
            t.type = T_R_PAREN;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case ':':
            t.type = T_COLON;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '<':
            t.start = &m_code[m_current];
            if (m_current + 1 < m_code.size() && m_code[m_current + 1] == '=') {
                t.type = T_LESS_EQUAL;
                t.len = 2;
            } else {
                t.type = T_LESS;
                t.len = 1;
            }
            break;
        case '>':
            t.start = &m_code[m_current];
            if (m_current + 1 < m_code.size() && m_code[m_current + 1] == '=') {
                t.type = T_GREATER_EQUAL;
                t.len = 2;
            } else {
                t.type = T_GREATER;
                t.len = 1;
            }
            break;
        case '=':
            t.start = &m_code[m_current];
            if (m_current + 1 < m_code.size() && m_code[m_current + 1] == '=') {
                t.type = T_EQUAL_EQUAL;
                t.len = 2;
            } else {
                t.type = T_EQUAL;
                t.len = 1;
            }
            break;
        case '!':
            t.start = &m_code[m_current];
            if (m_current + 1 < m_code.size() && m_code[m_current + 1] == '=') {
                t.type = T_NOT_EQUAL;
                t.len = 2;
            } else {
                t.type = T_NOT;
                t.len = 1;
            }
            break;
        case '{':
            t.type = T_L_BRACE;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '}':
            t.type = T_R_BRACE;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '[':
            t.type = T_L_BRACKET;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case ']':
            t.type = T_R_BRACKET;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case ',':
            t.type = T_COMMA;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        case '$':
            t.type = T_DOLLAR;
            t.start = &m_code[m_current];
            t.len = 1;
            break;
        default:
            if (is_digit(c)) {
                t = read_number();
            } else {
                t = read_word();
            }
            break;
    }
    m_current += t.len;
    return t;
}
