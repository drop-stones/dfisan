// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include "runtime_info.h"
#include <assert.h>
#include <stdio.h>

struct AlignFour {
  int i;
  long l;
};

struct NotAlignFour {
  char c1;
  char c2;
};

struct AlignFour gAlign1 __attribute__((annotate("dfi_protection")));
struct AlignFour gAlign2 __attribute__((annotate("dfi_protection")));
struct NotAlignFour gNotAlign1 __attribute__((annotate("dfi_protection")));
struct NotAlignFour gNotAlign2 __attribute__((annotate("dfi_protection")));

int main(void) {
  // Check global alignment.
  assert(((size_t)&gAlign1 == AlignAddr(&gAlign1) && (size_t)&gAlign2 == AlignAddr(&gAlign2)) && "gAlign is not 4 bytes aligned.");
  assert(((size_t)&gNotAlign1 == AlignAddr(&gNotAlign1) && (size_t)&gNotAlign2 == AlignAddr(&gNotAlign2)) && "gNotAlign is not 4 bytes aligned.");
  printf("&gAlign1 = %zx\n&gAlign2 = %zx\n", &gAlign1, &gAlign2);
  printf("&gNotAlign1 = %zx\n&gNotAlign2 = %zx\n", &gNotAlign1, &gNotAlign2);
  
  // Check local alignment.
  struct AlignFour lAlign1 __attribute__((annotate("dfi_protection")));
  struct AlignFour lAlign2 __attribute__((annotate("dfi_protection")));
  struct NotAlignFour lNotAlign1 __attribute__((annotate("dfi_protection")));
  struct NotAlignFour lNotAlign2 __attribute__((annotate("dfi_protection")));
  assert(((size_t)&lAlign1 == AlignAddr(&lAlign1) && (size_t)&lAlign2 == AlignAddr(&lAlign2)) && "lAlign is not 4 bytes aligned.");
  assert(((size_t)&lNotAlign1 == AlignAddr(&lNotAlign1) && (size_t)&lNotAlign2 == AlignAddr(&lNotAlign2)) && "lNotAlign is not 4 bytes aligned.");

  // Check heap alignment.
  struct AlignFour *hAlign1 = (struct AlignFour *)safe_malloc(sizeof(struct AlignFour));
  struct AlignFour *hAlign2 = (struct AlignFour *)safe_malloc(sizeof(struct AlignFour));
  struct NotAlignFour *hNotAlign1 = (struct NotAlignFour *)safe_malloc(sizeof(struct NotAlignFour));
  struct NotAlignFour *hNotAlign2 = (struct NotAlignFour *)safe_malloc(sizeof(struct NotAlignFour));
  assert(((size_t)hAlign1 == AlignAddr(hAlign1) && (size_t)hAlign2 == AlignAddr(hAlign2)) && "hAlign is not 4 bytes aligned.");
  assert(((size_t)hNotAlign1 == AlignAddr(hNotAlign1) && (size_t)hNotAlign2 == AlignAddr(hNotAlign2)) && "hNotAlign is not 4 bytes aligned.");

  free(hAlign1); free(hAlign2);
  free(hNotAlign1); free(hNotAlign2);

  return 0;
}
