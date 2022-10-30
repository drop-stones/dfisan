// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for analysis of def-use of aligned only targets.

#include <stdlib.h>

/// Safe malloc
void *safe_malloc(size_t size);

struct Unaligned {
  char c1, c2;
};

struct Unaligned g __attribute__((annotate("dfi_protection")));

int main(void) {
  struct Unaligned l __attribute__((annotate("dfi_protection"))) = { 'a', 'b' };
  struct Unaligned *h = (struct Unaligned *)safe_malloc(sizeof(struct Unaligned));
  h->c1 = 'c'; h->c2 = 'd';

  l.c1; l.c2;
  h->c1; h->c2;
  g.c1; g.c2;

  return 0;
}
