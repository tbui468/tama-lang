#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "byte_array.hpp"
#include "memory.hpp"

void ba_init(struct ByteArray* ba) {
    ba->bytes = NULL;
    ba->count = 0;
    ba->max_count = 0;
}

void ba_free(struct ByteArray* ba) {
    free_arr(ba->bytes, sizeof(uint8_t), ba->max_count);
}

void ba_append(struct ByteArray* ba, uint8_t* s, int len) {
    int old_max = ba->max_count;
    if (ba->max_count == 0) {
        ba->max_count = 8;
        ba->bytes = (uint8_t*)alloc_arr(ba->bytes, sizeof(uint8_t), old_max, ba->max_count);
    }

    while (ba->count + len > ba->max_count) {
        old_max = ba->max_count;
        ba->max_count *= 2;
        ba->bytes = (uint8_t*)alloc_arr(ba->bytes, sizeof(uint8_t), old_max, ba->max_count);
    }

    memcpy(&ba->bytes[ba->count], s, len);
    ba->count += len;
}

void ba_append_byte(struct ByteArray *ba, uint8_t b) {
    ba_append(ba, (uint8_t*)&b, 1);
}

void ba_append_word(struct ByteArray *ba, uint16_t w) {
    ba_append(ba, (uint8_t*)&w, 2);
}

void ba_append_double(struct ByteArray *ba, uint32_t d) {
    ba_append(ba, (uint8_t*)&d, 4);
}
