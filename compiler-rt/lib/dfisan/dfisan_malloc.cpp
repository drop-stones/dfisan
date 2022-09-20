#include "dfisan_malloc.h"
#include "dfisan_interface_internal.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

using namespace __sanitizer;

// globals
void *unsafe_heap, *safe_aligned_heap, *safe_unaligned_heap;
mspace unsafe_region, safe_aligned_region, safe_unaligned_region;

// Initialization functions of memory layout
void* ReserveRegion(size_t beg, size_t end, const char *name) {
  ReserveShadowMemoryRange(beg, end, name); // mmap needs MAP_NORESERVE
  return (void*)beg;
}

void ReserveUnsafeRegion(size_t beg, size_t end) {
  size_t size = end - beg + 1;
  unsafe_heap = ReserveRegion(beg, end, "unsafe heap");
  unsafe_region = create_mspace_with_base(unsafe_heap, size, 0);
  Report("INFO: Reserve Unsafe Region (0x%zx, 0x%zx)\n", beg, end);
}

void ReserveSafeAlignedRegion(size_t mem_beg, size_t mem_end, size_t shadow_beg, size_t shadow_end) {
  size_t mem_size = mem_end - mem_beg + 1;
  size_t shadow_size = shadow_end - shadow_beg + 1;
  CHECK_EQ(mem_size, shadow_size * 2);
  safe_aligned_heap = ReserveRegion(mem_beg, mem_end, "safe aligned heap");
  safe_aligned_region = create_mspace_with_base(safe_aligned_heap, mem_size, 0);
  Report("INFO: Reserve Safe Aligned Region (0x%zx, 0x%zx)\n", mem_beg, mem_end);

  ReserveShadowMemoryRange(shadow_beg, shadow_end, "safe aligned shadow");
  Report("INFO: Reserve Shadow Aligned Region (0x%zx, 0x%zx)\n", shadow_beg, shadow_end);
}

void ReserveSafeUnalignedRegion(size_t mem_beg, size_t mem_end, size_t shadow_beg, size_t shadow_end) {
  size_t mem_size = mem_end - mem_beg + 1;
  size_t shadow_size = shadow_end - shadow_beg + 1;
  CHECK_EQ(mem_size * 2, shadow_size);
  safe_unaligned_heap = ReserveRegion(mem_beg, mem_end, "safe unaligned heap");
  safe_unaligned_region = create_mspace_with_base(safe_unaligned_heap, mem_size, 0);
  Report("INFO: Reserve Safe Unaligned Region (0x%zx, 0x%zx)\n", mem_beg, mem_end);

  ReserveShadowMemoryRange(shadow_beg, shadow_end, "safe unaligned shadow");
  Report("INFO: Reserve Shadow Unaligned Region (0x%zx, 0x%zx)\n", shadow_beg, shadow_end);
}

/// Functions for interception
// mallocs
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void* __dfisan_unsafe_malloc(size_t n) {
  return mspace_malloc(unsafe_region, n);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void* __dfisan_safe_aligned_malloc(size_t n) {
  return mspace_malloc(safe_aligned_region, n);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void* __dfisan_safe_unaligned_malloc(size_t n) {
  return mspace_malloc(safe_unaligned_region, n);
}

// frees
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unsafe_free(void *ptr) {
  mspace_free(unsafe_region, ptr);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_safe_aligned_free(void *ptr) {
  mspace_free(safe_aligned_region, ptr);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_safe_unaligned_free(void *ptr) {
  mspace_free(safe_unaligned_region, ptr);
}

// callocs
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_unsafe_calloc(size_t n, size_t elem_size) {
  return mspace_calloc(unsafe_region, n, elem_size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_safe_aligned_calloc(size_t n, size_t elem_size) {
  return mspace_calloc(safe_aligned_region, n, elem_size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_safe_unaligned_calloc(size_t n, size_t elem_size) {
  return mspace_calloc(safe_unaligned_region, n, elem_size);
}

// reallocs
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_unsafe_realloc(void *ptr, size_t n) {
  return mspace_realloc(unsafe_region, ptr, n);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_safe_aligned_realloc(void *ptr, size_t n) {
  return mspace_realloc(safe_aligned_region, ptr, n);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_safe_unaligned_realloc(void *ptr, size_t n) {
  return mspace_realloc(safe_unaligned_region, ptr, n);
}

// mmap
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void *__dfisan_unsafe_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  void *rv = mmap(addr, length, prot, flags, fd, offset);
  if (rv == MAP_FAILED) {
    fprintf(stderr, "ERROR: mmap %d - %s\n", errno, strerror(errno));
  }
  return rv;
}
