#ifndef TYPE_HPP
#define TYPE_HPP

#include <vector>
#include "token.hpp"

class Type {
    public:
        //if m_dtype == T_FUN_TYPE, then m_rtype and m_params can be used to find function type
        enum TokenType m_dtype;
        enum TokenType m_rtype;
        std::vector<Type> m_ptypes;
    public:
        Type(enum TokenType dtype, enum TokenType rtype, std::vector<Type> params):
            m_dtype(dtype), m_rtype(rtype), m_ptypes(params) {}
        Type(enum TokenType dtype): m_dtype(dtype), m_rtype(T_NIL_TYPE), m_ptypes(std::vector<Type>()) {}
        bool is_of_type(const Type& other) {
            if (m_dtype != other.m_dtype) return false;
            if (m_ptypes.size() != other.m_ptypes.size()) return false;

            if (m_dtype == T_FUN_TYPE) {
                for (int i = 0; i < int(m_ptypes.size()); i++) {
                    if (!m_ptypes.at(i).is_of_type(other.m_ptypes.at(i))) return false;
                }
            }

            return true;
        }
};

#endif //TYPE_HPP
