// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage array initialization.

struct Aligned { int i; long l; };
struct Unaligned { char c1, c2; };

int garr[4] __attribute__((annotate("dfi_protection"))) = { 100 };
struct Aligned gAligned[4] __attribute__((annotate("dfi_protection"))) = { { 200, 300 } };
struct Unaligned gUnaligned[4] __attribute__((annotate("dfi_protection"))) = { { 'a', 'b' } };

int main(void) {
  // check global array
  garr[0]; garr[3];
  gAligned[0].i; gAligned[0].l; gAligned[3].i; gAligned[1].l;
  gUnaligned[0].c1; gUnaligned[0].c2; gUnaligned[3].c1; gUnaligned[3].c2;

  // check local array
  int larr[4] __attribute__((annotate("dfi_protection"))) = { 100 };
  struct Aligned lAligned[4] __attribute__((annotate("dfi_protection"))) = { { 200, 300 } };
  struct Unaligned lUnaligned[4] __attribute__((annotate("dfi_protection"))) = { { 'a', 'b' } };
  larr[0]; larr[3];
  lAligned[0].i; lAligned[0].l; lAligned[3].i; lAligned[1].l;
  lUnaligned[0].c1; lUnaligned[0].c2; lUnaligned[3].c1; lUnaligned[3].c2;
  
  return 0;
}
