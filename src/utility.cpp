#include "utility.hpp"

bool is_int(const std::string s) {
    if (s[0] == '-') {
        return s.substr(1, s.size() - 1).find_first_not_of("0123456789") == std::string::npos;
    }
    return s.find_first_not_of("0123456789") == std::string::npos; 
}
