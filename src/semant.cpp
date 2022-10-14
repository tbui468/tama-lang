#include <fstream>
#include <sstream>
#include <iostream>

#include "semant.hpp"

std::vector<Ast*> Semant::generate_ast(const std::string& input_file) {
    read(input_file);
    lex();
    parse();
    type_check();
    return m_nodes;
}

void Semant::read(const std::string& input_file) {
    std::ifstream f(input_file);
    std::stringstream buffer;
    buffer << f.rdbuf();
    m_code = buffer.str();
}

void Semant::lex() {
    m_tokens = m_lexer.lex(m_code, m_reserved_words);
    for (struct Token t: m_tokens) {
        printf("%.*s\n", t.len, t.start);
    }
}

void Semant::parse() {
}

void Semant::type_check() {

}
