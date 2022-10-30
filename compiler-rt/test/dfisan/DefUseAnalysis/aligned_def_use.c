// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for analysis of def-use of aligned targets.

#include "../safe_alloc.h"

struct Aligned {
  int x, y;
};

int main(int argc, char **argv) {
  struct Aligned *alignedOnly = (argc == 1) ? (struct Aligned *)safe_malloc(sizeof(struct Aligned))
                                            : (struct Aligned *)safe_calloc(1, sizeof(struct Aligned));
  alignedOnly->x = 100; alignedOnly->y = 200; // AlignedOnly Def
  alignedOnly->x; alignedOnly->y;             // AlignedOnly Use

  struct Aligned *alignedOrNoTarget = (argc == 1) ? (struct Aligned *)safe_malloc(sizeof(struct Aligned))
                                                  : (struct Aligned *)malloc(sizeof(struct Aligned));
  alignedOrNoTarget->x = 300; alignedOrNoTarget->y = 400; // AlignedOrNoTarget Def
  alignedOrNoTarget->x; alignedOrNoTarget->y;             // AlignedOrNoTarget Use

  return 0;
}
