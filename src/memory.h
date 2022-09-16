#ifndef TMD_MEMORY_H
#define TMD_MEMORY_H

#include <stddef.h>

void* alloc_arr(void *vptr, size_t unit_size, int old_count, int new_count);
void* alloc_unit(size_t unit_size);
void free_arr(void *vtpr, size_t unit_size, int count);
void free_unit(void *vptr, size_t unit_size);

#endif //TMD_MEMORY_H
