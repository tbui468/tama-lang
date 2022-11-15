#ifndef TAC_HPP
#define TAC_HPP

#include <iostream>
#include <string>
#include "token.hpp"
#include "type.hpp"

struct EmitTacResult {
    std::string m_temp;
    Type m_type;
};

class TacQuad {
    public:
        std::string m_target;
        std::string m_opd1;
        std::string m_opd2;
        enum TokenType m_op;
        static int s_label_counter;
    public:

        static std::string new_label() {
            return "_L" + std::to_string(s_label_counter++);
        }
        TacQuad(const std::string& target, const std::string& opd1, const std::string& opd2, enum TokenType op):
            m_target(target), m_opd1(opd1), m_opd2(opd2), m_op(op) {}

        std::string to_string() const {

            if (m_target == "" || m_op == T_NIL) {
                return m_target + (m_target == "" ? "" : " ")  + m_opd1 + " " + m_opd2; 
            } else {
                std::string ret = m_target + " = " + m_opd1;
                switch (m_op) {
                    case T_PLUS:        ret += " + ";   break; 
                    case T_MINUS:       ret += " - ";   break; 
                    case T_STAR:        ret += " * ";   break; 
                    case T_SLASH:       ret += " / ";   break; 
                    case T_EQUAL_EQUAL: ret += " == ";  break; 
                    case T_LESS:        ret += " < ";   break; 
                    case T_OR:          ret += " || ";  break; 
                    case T_AND:         ret += " && ";  break; 
                    default:            ret += " "; break;
                }
                return ret += m_opd2;
            }
        }

    static void print_tac(const std::vector<TacQuad>& quads, const std::vector<std::string>& tac_labels) {
        for (int i = 0; i < quads.size(); i++) {
            const TacQuad& q = quads[i];
            const std::string& str = tac_labels[i];
            if (str != "") {
                std::cout << str << ":" << std::endl;
            }
            std::cout << "    " << q.to_string() << std::endl;
        }
    }

};

#endif //TAC_HPP
