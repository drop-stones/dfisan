// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for analysis of def-use of aligned and unaligned targets.

#include <stdlib.h>

/// Safe malloc
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);

struct Aligned {
  int x, y;
};

struct Unaligned {
  char c1, c2;
  int i;
};

int main(int argc, char **argv) {
  struct Aligned aligned __attribute__((annotate("dfi_protection"))) = {100, 200};
  struct Unaligned unaligned __attribute__((annotate("dfi_protection"))) = {'a', 'b', 300};
  int noTarget = 400;

  int *bothOnly = (argc == 1) ? &aligned.x : &unaligned.i;
  *bothOnly = 500;
  *bothOnly;

  int *bothOrNoTarget = (argc == 1) ? &aligned.x : (argc == 2) ? &unaligned.i : &noTarget;
  *bothOrNoTarget = 600;
  *bothOrNoTarget;

  return 0;
}

