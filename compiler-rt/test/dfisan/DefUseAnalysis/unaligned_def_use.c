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
  struct Unaligned *alignedOnly = (argc == 1) ? (struct Unaligned *)safe_malloc(sizeof(struct Unaligned))
                                              : (struct Unaligned *)safe_calloc(1, sizeof(struct Unaligned));
  alignedOnly->c = 'a'; alignedOnly->s = 100; // UnalignedOnly Def
  alignedOnly->c; alignedOnly->s;             // UnalignedOnly Use

  struct Unaligned *alignedOrNoTarget = (argc == 1) ? (struct Unaligned *)safe_malloc(sizeof(struct Unaligned))
                                                    : (struct Unaligned *)malloc(sizeof(struct Unaligned));
  alignedOrNoTarget->c = 'b'; alignedOrNoTarget->s = 200; // UnalignedOrNoTarget Def
  alignedOrNoTarget->c; alignedOrNoTarget->s;             // UnalignedOrNoTarget Use

  return 0;
}
