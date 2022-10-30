// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for analysis of def-use of aligned only targets.

#include <stdlib.h>
#include "../safe_alloc.h"

struct Aligned {
  int x, y;
};

struct Aligned g __attribute__((annotate("dfi_protection")));

int main(void) {
  int i __attribute__((annotate("dfi_protection"))) = 100;
  struct Aligned l __attribute__((annotate("dfi_protection"))) = { 200, 300 };
  struct Aligned *h = (struct Aligned *)safe_malloc(sizeof(struct Aligned));
  h->x = 400; h->y = 500;

  i;
  l.x; l.y;
  h->x; h->y;
  g.x; g.y;

  return 0;
}
