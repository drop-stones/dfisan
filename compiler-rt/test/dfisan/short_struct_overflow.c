// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in short.

struct ShortSet {
  short s0[1];
  short s1;
};

int main(int argc, char **argv) {
  struct ShortSet ss;
  ss.s1 = 100;
  ss.s0[2] = 200;   // overwrite ss.s1

  ss.s1;            // detect
  return 0;
}