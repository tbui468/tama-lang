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


enum class TacT {
    EmptyQuad,
    Plus,
    Minus,
    Star,
    Slash,
    Assign,
    EqualEqual,
    Less,
    Or,
    And,
    Goto,
    CondGoto,
    Entry,
    Exit,
    FunBegin,
    FunEnd,
    PushArg,
    PopArgs,
    CallResult,
    CallNil,
    Return
};

class TacQuad {
    public:
    public:
        std::string m_target;
        std::string m_opd1;
        std::string m_opd2;
        enum TacT m_op;
        static int s_label_counter;
    public:

        static std::string new_label() {
            return "_L" + std::to_string(s_label_counter++);
        }
        TacQuad(const std::string& target, const std::string& opd1, const std::string& opd2, enum TacT op):
            m_target(target), m_opd1(opd1), m_opd2(opd2), m_op(op) {}

        
        std::string to_string() const {
            std::string ret;
            switch (m_op) {
                case TacT::EmptyQuad: ret = "EmptyQuad"; break;
                case TacT::Plus: ret = "Plus"; break;
                case TacT::Minus: ret = "Minus"; break;
                case TacT::Star: ret = "Star"; break;
                case TacT::Slash: ret = "Slash"; break;
                case TacT::Assign: ret = "Assign"; break;
                case TacT::EqualEqual: ret = "EqualEqual"; break;
                case TacT::Less: ret = "Less"; break;
                case TacT::Or: ret = "Or"; break;
                case TacT::And: ret = "And"; break;
                case TacT::Goto: ret = "Goto"; break;
                case TacT::CondGoto: ret = "CondGoto"; break;
                case TacT::Entry: ret = "Entry"; break;
                case TacT::Exit: ret = "Exit"; break;
                case TacT::FunBegin: ret = "FunBegin"; break;
                case TacT::FunEnd: ret = "FunEnd"; break;
                case TacT::PushArg: ret = "PushArg"; break;
                case TacT::PopArgs: ret = "PopArgs"; break;
                case TacT::CallResult: ret = "CallResult"; break;
                case TacT::CallNil: ret = "CallNil"; break;
                case TacT::Return: ret = "Return"; break;
                default: ret = "<Unrecognized TacT>"; break;
            }

            ret += ", " + m_target + ", " + m_opd1 + ", " + m_opd2;
            return ret;
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
