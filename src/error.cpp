#include <cstdio>
#include <stdarg.h>
#include <iostream>

#include "error.hpp"

ErrorMsgs ems;

void ErrorMsgs::add_error(int line, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char* s = (char*)malloc(256);
    int written = snprintf(s, 256, "[%d] ", line);
    vsnprintf(s + written, 256 - written, format, ap);
    va_end(ap);

    m_errors.push_back({std::string(s), line});
}

void ErrorMsgs::sort() {
    for (int end = m_errors.size() - 1; end > 0; end--) {
        for (int i = 0; i < end; i++) {
            Error left = m_errors[i];
            Error right = m_errors[i + 1];
            if (left.line > right.line) {
                m_errors[i] = right;
                m_errors[i + 1] = left;
            }
        }
    }
}

void ErrorMsgs::print() {
    for (const Error& e: m_errors) {
        std::cout << e.msg << std::endl;
    }
}

bool ErrorMsgs::has_errors() {
    return !m_errors.empty();
}


