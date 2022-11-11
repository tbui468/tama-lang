#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include "token.hpp"
#include "tac.hpp"
#include "type.hpp"

class Semant;

class Ast {
    public:
        virtual std::string to_string() = 0;
        virtual Type translate(Semant& s) = 0;
        virtual EmitTacResult emit_ir(Semant& s) = 0;
};


#endif //AST_HPP
