#ifndef TMD_ERROR_HPP
#define TMD_ERROR_HPP

#include <vector>
#include <string>

struct Error {
    std::string msg;
    int line;
};

class ErrorMsgs {
    public:
        std::vector<Error> m_errors;
    public:
        void add_error(int line, const char* format, ...);
        void sort();
        void print();
        bool has_errors();
};

extern ErrorMsgs ems;


#endif //TMD_ERROR_HPP
