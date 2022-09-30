#include <stdio.h>
#include <stdlib.h>

size_t allocated = 0;

void* alloc_arr(void *vptr, size_t unit_size, int old_count, int new_count) {
    allocated += unit_size * (new_count - old_count);
    void* ret;
    
    if (!(ret = realloc(vptr, unit_size * new_count))) {
        printf("System Error: realloc failed\n");
        exit(1);
    }

    return ret;
}

void* alloc_unit(size_t unit_size) {
    return alloc_arr(NULL, unit_size, 0, 1);
}

void free_arr(void *vtpr, size_t unit_size, int count) {
    free(vtpr);
    allocated -= unit_size * count;
}

void free_unit(void *vptr, size_t unit_size) {
    free_arr(vptr, unit_size, 1);
}
