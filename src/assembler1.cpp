#include "assembler1.hpp"
#include <fstream>
#include <iostream>
#include <sstream>


void Assembler1::emit_code(const std::string& input_file, const std::string& output_file) {
    lex(input_file);
    parse();
    assemble();
    write(output_file);
}

void Assembler1::lex(const std::string& input_file) {
    std::ifstream f(input_file);
    std::stringstream buffer;
    buffer << f.rdbuf();
//    std::cout << buffer.str() << '\n';
    std::cout << "Done" << '\n';
}

void Assembler1::parse() {

}

void Assembler1::assemble() {

}

void Assembler1::write(const std::string& output_file) {

}
