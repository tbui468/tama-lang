#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <string>
#include <vector>

//#include "token.h"
//#include "ast.h"


class Assembler1 {
    public:
        void emit_code(const std::string& input_file, const std::string& output_file);
    private:
        void lex(const std::string& input_file);
        void parse();
        void assemble();
        void write(const std::string& output_file);
    private:
        //std::vector<struct Token> m_tokens;
        //std::vector<struct Node*> m_nodes;
        //std::vector<uint8_t> m_buf;
};


#endif //ASSEMBLER_HPP
