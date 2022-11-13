#include "utility.hpp"

bool is_int(const std::string s) {
    return s.find_first_not_of("0123456789") == std::string::npos; 
}
