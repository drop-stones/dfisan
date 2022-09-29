// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage all primitive types.

struct S {
  char c;
  unsigned char uc;
  short s;
  unsigned short us;
  int i;
  unsigned int ui;
  long l;
  unsigned long ul;
  float f;
  double d;
  long double ld;
  __float128 f128;
};

int main(int argc, char **argv) {
  char c            __attribute__((annotate("dfi_protection"))) = 'a';
  unsigned char uc  __attribute__((annotate("dfi_protection"))) = 'b';
  short s           __attribute__((annotate("dfi_protection"))) = 100;
  unsigned short us __attribute__((annotate("dfi_protection"))) = 200;
  int i             __attribute__((annotate("dfi_protection"))) = 300;
  unsigned int ui   __attribute__((annotate("dfi_protection"))) = 400;
  long l            __attribute__((annotate("dfi_protection"))) = 500;
  unsigned long ul  __attribute__((annotate("dfi_protection"))) = 600;
  float f           __attribute__((annotate("dfi_protection"))) = 1.23;
  double d          __attribute__((annotate("dfi_protection"))) = 4.56;
  long double ld    __attribute__((annotate("dfi_protection"))) = 7.89;
  __float128 f128   __attribute__((annotate("dfi_protection"))) = 10.11;
  struct S st __attribute__((annotate("dfi_protection"))) = { 'c', 'd', 1, 2, 3, 4, 5, 6, 7.8, 9.1, 1.2, 3.4 };

  // Use all variables
  long double sum = c + uc + s + us + i + ui + l + ul + f + d + ld + f128
                  + st.c + st.uc + st.s + st.us + st.i + st.ui + st.l + st.ul + st.f + st.d + st.ld + st.f128;

  return 0;
}