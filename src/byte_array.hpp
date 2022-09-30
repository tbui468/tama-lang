#ifndef TMD_BYTE_ARRAY_H
#define TMD_BYTE_ARRAY_H

struct ByteArray {
    uint8_t* bytes;
    int count;
    int max_count;
};


void ba_init(struct ByteArray* ba);
void ba_free(struct ByteArray* ba);
void ba_append(struct ByteArray* ba, uint8_t* s, int len);
void ba_append_byte(struct ByteArray *ba, uint8_t b);
void ba_append_word(struct ByteArray *ba, uint16_t w);
void ba_append_double(struct ByteArray *ba, uint32_t d);

#endif //TMD_BYTE_ARRAY_H
