#include <stdlib.h>

#include "token.h"

uint32_t get_double(struct Token t) {
    char* end = t.start + t.len;
    long ret;
    if (t.type == T_INT) {
        ret = strtol(t.start, &end, 10);
    } else if (t.type == T_HEX) {
        ret = strtol(t.start + 2, &end, 16);
    }
    return (int32_t)ret;
}

uint8_t get_byte(struct Token t) {
    char* end = t.start + t.len;
    long ret;
    if (t.type == T_INT) {
        ret = strtol(t.start, &end, 10);
    } else if (t.type == T_HEX) {
        ret = strtol(t.start + 2, &end, 16);
    }
    return (uint8_t)ret;
}
