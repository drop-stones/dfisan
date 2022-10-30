// RUN: %clang_dfisan %s -o %t -mllvm -unaligned-region-only
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan place all targets in unaligned region if `-unaligned-region-only` flag is on.

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

struct AlignFour gAlign __attribute__((annotate("dfi_protection")));
struct NotAlignFour gNotAlign __attribute__((annotate("dfi_protection")));
struct AlignFour gUnsafe;

int main(void) {
  // Check global struct
  assert(AddrIsInSafeUnalignedGlobal(&gAlign) && "gAlign is not in safe unaligned region");
  assert(AddrIsInSafeUnalignedGlobal(&gNotAlign) && "gNotAlign is not in safe unaligned region");
  assert(AddrIsInUnsafeRegion(&gUnsafe) && "gUnsafe is not in unsafe region");

  // Check local struct
  struct AlignFour lAlign __attribute__((annotate("dfi_protection")));
  struct NotAlignFour lNotAlign __attribute__((annotate("dfi_protection")));
  struct AlignFour lUnsafe;
  assert(AddrIsInSafeUnalignedHeap(&lAlign) && "lAlign is not in safe unaligned region");
  assert(AddrIsInSafeUnalignedHeap(&lNotAlign) && "lNotAlign is not in safe unaligned region");
  assert(AddrIsInUnsafeRegion(&lUnsafe) && "lUnsafe is not in unsafe region");

  // Check heap struct
  struct AlignFour *hAlign = (struct AlignFour *)safe_malloc(sizeof(struct AlignFour));
  struct NotAlignFour *hNotAlign = (struct NotAlignFour *)safe_malloc(sizeof(struct NotAlignFour));
  struct AlignFour *hUnsafe = (struct AlignFour *)malloc(sizeof(struct AlignFour));
  assert(AddrIsInSafeUnalignedHeap(hAlign) && "hAlign is not in safe unaligned region!!");
  assert(AddrIsInSafeUnalignedHeap(hNotAlign) && "hNotAlign is not in safe unaligned region!!");
  assert(AddrIsInUnsafeRegion(hUnsafe) && "hUnsafe is not in unsafe region!!");

  free(hAlign);
  free(hNotAlign);
  free(hUnsafe);

  return 0;
}
