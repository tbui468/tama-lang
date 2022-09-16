#include <stdio.h>
#include <stdarg.h>

#include "error.h"
#include "memory.h"

#define MAX_MSG_LEN 256

void ems_init(struct ErrorMsgs *ems) {
    ems->errors = NULL;
    ems->count = 0;
    ems->max_count = 0;
}

void ems_free(struct ErrorMsgs *ems) {
    for (int i = 0; i < ems->count; i++) {
        free_unit(ems->errors[i].msg, MAX_MSG_LEN);
    }
    free_arr(ems->errors, sizeof(struct Error), ems->max_count);
}

void ems_add(struct ErrorMsgs *ems, int line, char* format, ...) {
    if (ems->count + 1 > ems->max_count) {
        int old_max = ems->max_count;
        if (ems->max_count == 0) {
            ems->max_count = 8;
        } else {
            ems->max_count *= 2;
        }
        ems->errors = alloc_arr(ems->errors, sizeof(struct Error), old_max, ems->max_count);
    }

    va_list ap;
    va_start(ap, format);
    char* s = alloc_unit(MAX_MSG_LEN);
    int written = snprintf(s, MAX_MSG_LEN, "[%d] ", line);
    vsnprintf(s + written, MAX_MSG_LEN - written, format, ap);
    va_end(ap);

    struct Error e;
    e.msg = s;
    e.line = line;

    ems->errors[ems->count++] = e;
}

void ems_sort(struct ErrorMsgs *ems) {
    for (int end = ems->count - 1; end > 0; end--) {
        for (int i = 0; i < end; i++) {
            struct Error left = ems->errors[i];
            struct Error right = ems->errors[i + 1];
            if (left.line > right.line) {
                ems->errors[i] = right;
                ems->errors[i + 1] = left;
            }
        }
    }
}

void ems_print(struct ErrorMsgs *ems) {
    ems_sort(ems);
    for (int i = 0; i < ems->count; i++) {
        printf("%s\n", ems->errors[i].msg); 
    }
}
