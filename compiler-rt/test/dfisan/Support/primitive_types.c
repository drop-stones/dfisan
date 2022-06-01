// RUN: %clang_dfisan %s -o %t && %run %t
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
  char c = 'a';
  unsigned char uc = 'b';
  short s = 100;
  unsigned short us = 200;
  int i = 300;
  unsigned int ui = 400;
  long l = 500;
  unsigned long ul = 600;
  float f = 1.23;
  double d = 4.56;
  long double ld = 7.89;
  __float128 f128 = 10.11;

  // Use all variables
  long double sum = c + uc + s + us + i + ui + l + ul + f + d + ld + f128;

  return 0;
}