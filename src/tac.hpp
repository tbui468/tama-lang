#ifndef TAC_HPP
#define TAC_HPP

#include <string>
#include "token.hpp"

class TacQuad {
    public:
        std::string m_target;
        std::string m_opd1;
        std::string m_opd2;
        enum TokenType m_op;
        static int s_temp_counter;
    public:
        static std::string new_temp() {
            return "_t" + std::to_string(s_temp_counter++);
        }
        TacQuad(const std::string& target, const std::string& opd1, const std::string& opd2, enum TokenType op):
            m_target(target), m_opd1(opd1), m_opd2(opd2), m_op(op) {}
        std::string to_string() const {
            std::string ret = m_target;
            switch (m_op) {
                case T_EQUAL: {
                    ret += " = " + m_opd1;
                    return ret;
                }
                case T_PLUS: {
                    ret += " = " + m_opd1 + " + " + m_opd2;
                    return ret;
                }
                case T_MINUS: {
                    ret += " = " + m_opd1 + " - " + m_opd2;
                    return ret;
                }
                case T_STAR: {
                    ret += " = " + m_opd1 + " * " + m_opd2;
                    return ret;
                }
                case T_SLASH: {
                    ret += " = " + m_opd1 + " / " + m_opd2;
                    return ret;
                }
                case T_NIL: {
                    if (m_target == "begin_fun") {
                        ret += " " + m_opd1;
                    }
                    return ret;
                }
                default:
                    return ret;
            }
        }
};

#endif //TAC_HPP
