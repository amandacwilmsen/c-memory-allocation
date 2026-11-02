#include <stdlib.h>
#include "mymemory.h"

mymemory_t* mymemory_init(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    mymemory_t *memory = malloc(sizeof(mymemory_t));

    if (memory == NULL) {
        return NULL;
    }

    memory->pool = malloc(size);

    if (memory->pool == NULL) {
        free(memory);
        return NULL;
    }

    memory->total_size = size;
    memory->head = NULL;

    return memory;
}
