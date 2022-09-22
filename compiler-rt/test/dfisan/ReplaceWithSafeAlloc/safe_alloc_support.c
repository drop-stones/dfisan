// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include "runtime_info.h"
#include <assert.h>

int main(void) {
  char *str = (char *)safe_malloc(100);
  assert(AddrIsInSafeAlignedHeap(str) && "str is not in safe aligned region!!");

  str = (char *)safe_realloc(str, 200);
  assert(AddrIsInSafeAlignedHeap(str) && "str is not in safe aligned region!!");

  int *p = (int *)safe_calloc(200, sizeof(long));
  assert(AddrIsInSafeAlignedHeap(p) && "p is not in safe aligned region!!");

  free(str);
  free(p);

  return 0;
}
