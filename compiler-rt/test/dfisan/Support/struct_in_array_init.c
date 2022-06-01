// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage struct initialization in array.

struct S0 {
  int i;
};

int main(void) {
  struct S0 arr[4] = { {100}, {200}, {300}, {400} };

  arr[0].i;
  arr[1].i;
  arr[2].i;
  arr[3].i;

  arr[0].i = 500;
  arr[1].i = 600;
  arr[2].i = 700;
  arr[3].i = 800;

  arr[0].i;
  arr[1].i;
  arr[2].i;
  arr[3].i;

  return 0;
}