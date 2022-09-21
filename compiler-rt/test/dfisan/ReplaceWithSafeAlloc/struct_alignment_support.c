// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include "runtime_info.h"
#include <assert.h>

struct AlignFour {
  char c;   // offset: 0
  int i;    // offset: 4
  short s;  // offset: 8
  long l;   // offset: 16
};

struct NotAlignFour {
  char c;   // offset: 0
  short s;  // offset: 2 <- not 4 aligned
  int i;    // offset: 4
  long l;   // offset: 8
};

int main(void) {
  struct AlignFour *align = (struct AlignFour *)safe_malloc(sizeof(struct AlignFour));
  assert(AddrIsInSafeAligned(align) && "align is not in safe aligned region!!");

  struct NotAlignFour *notAlign = (struct NotAlignFour *)safe_malloc(sizeof(struct NotAlignFour));
  assert(AddrIsInSafeUnaligned(notAlign) && "notAlign is not in safe unaligned region!!");

  struct AlignFour *unsafe = (struct AlignFour *)malloc(sizeof(struct AlignFour));
  assert(AddrIsInUnsafeRegion(unsafe) && "unsafe is not in unsafe region!!");

  free(align);
  free(notAlign);
  free(unsafe);

  return 0;
}
