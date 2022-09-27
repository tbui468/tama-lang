#include <stdlib.h>

#include "token.h"
#include "memory.h"

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
    return (int8_t)ret;
}


void ta_init(struct TokenArray *ta) {
    ta->tokens = NULL;
    ta->count = 0;
    ta->max_count = 0;
}

void ta_free(struct TokenArray *ta) {
    free_arr(ta->tokens, sizeof(struct Token), ta->max_count); 
}

void ta_add(struct TokenArray *ta, struct Token t) {
    int old_max = ta->max_count;
    if (ta->max_count == 0) {
        ta->max_count = 8;
    }else if (ta->count + 1 > ta->max_count) {
        ta->max_count *= 2;
    }
    ta->tokens = alloc_arr(ta->tokens, sizeof(struct Token), old_max, ta->max_count);

    ta->tokens[ta->count] = t;
    ta->count++;
}
