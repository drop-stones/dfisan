/*
#define MSPACES 3
#define MSPACES_ONLY 1
#define USE_DL_PREFIX 1
*/

#include "dlmalloc/dlmalloc.h"

// mallocs
void* __dfisan_unsafe_malloc(size_t n);
void* __dfisan_safe_aligned_malloc(size_t n);
void* __dfisan_safe_unaligned_malloc(size_t n);
