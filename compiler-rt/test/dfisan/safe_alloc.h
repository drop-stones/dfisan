#ifndef SAFE_ALLOC_H
#define SAFE_ALLOC_H

#include <stddef.h>

/// Safe malloc
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);

#endif