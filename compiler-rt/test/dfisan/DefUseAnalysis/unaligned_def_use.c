// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for analysis of def-use of unaligned targets.

#include <stdlib.h>

/// Safe malloc
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);

struct Unaligned {
  char c;
  short s;
};

int main(int argc, char **argv) {
  struct Unaligned *unalignedOnly = (argc == 1) ? (struct Unaligned *)safe_malloc(sizeof(struct Unaligned))
                                                : (struct Unaligned *)safe_calloc(1, sizeof(struct Unaligned));
  unalignedOnly->c = 'a'; unalignedOnly->s = 100; // UnalignedOnly Def
  unalignedOnly->c; unalignedOnly->s;             // UnalignedOnly Use

  struct Unaligned *unalignedOrNoTarget = (argc == 1) ? (struct Unaligned *)safe_malloc(sizeof(struct Unaligned))
                                                      : (struct Unaligned *)malloc(sizeof(struct Unaligned));
  unalignedOrNoTarget->c = 'b'; unalignedOrNoTarget->s = 200; // UnalignedOrNoTarget Def
  unalignedOrNoTarget->c; unalignedOrNoTarget->s;             // UnalignedOrNoTarget Use

  return 0;
}
