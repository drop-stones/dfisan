// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan supports struct initialization.
//   - struct which have primitive type
//   - struct which have another struct
//   - struct which have arrays

struct Aligned { int i; long l; };
struct Unaligned { char c1, c2; };

struct HaveStruct {
  struct Aligned a;
  struct Unaligned u;
};

struct HaveArr {
  char str1[8];
  char str2[8];
};

// globals
struct Aligned gAligned __attribute__((annotate("dfi_protection"))) = { 1, 2 };
struct Unaligned gUnaligned __attribute__((annotate("dfi_protection"))) = { 'a', 'b' };
struct HaveStruct gHaveStruct __attribute__((annotate("dfi_protection"))) = { { 3, 4 }, { 'c', 'd' } };
struct HaveArr gHaveArr __attribute__((annotate("dfi_protection"))) = { "hello", "world" };

int main(void) {
  // check global struct
  gAligned.i; gAligned.l;
  gUnaligned.c1; gUnaligned.c2;
  gHaveStruct.a.i; gHaveStruct.a.l; gHaveStruct.u.c1; gHaveStruct.u.c2;
  gHaveArr.str1[0]; gHaveArr.str1[7]; gHaveArr.str2[0]; gHaveArr.str2[7];

  // check local struct
  struct Aligned lAligned __attribute__((annotate("dfi_protection"))) = { 1, 2 };
  struct Unaligned lUnaligned __attribute__((annotate("dfi_protection"))) = { 'a', 'b' };
  struct HaveStruct lHaveStruct __attribute__((annotate("dfi_protection"))) = { { 3, 4 }, { 'c', 'd' } };
  struct HaveArr lHaveArr __attribute__((annotate("dfi_protection"))) = { "hello", "world" };
  lAligned.i; lAligned.l;
  lUnaligned.c1; lUnaligned.c2;
  lHaveStruct.a.i; lHaveStruct.a.l; lHaveStruct.u.c1; lHaveStruct.u.c2;
  lHaveArr.str1[0]; lHaveArr.str1[7]; lHaveArr.str2[0]; lHaveArr.str2[7];

  return 0;
}
