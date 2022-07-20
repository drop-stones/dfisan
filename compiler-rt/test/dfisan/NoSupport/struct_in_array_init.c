// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage struct initialization in array.
// TODO: Support memcpy().

struct S0 {
  int i;
  long l;
};

int main(void) {
  struct S0 arr[4] = { {100, 200}, {300, 400}, {500, 600}, {700, 800} };

  arr[0].i;
  arr[0].l;
  arr[1].i;
  arr[1].l;
  arr[2].i;
  arr[2].l;
  arr[3].i;
  arr[3].l;

  arr[0].i = 1;
  arr[0].l = 2;
  arr[1].i = 3;
  arr[1].l = 4;
  arr[2].i = 5;
  arr[2].l = 6;
  arr[3].i = 7;
  arr[3].l = 8;

  arr[0].i;
  arr[0].l;
  arr[1].i;
  arr[1].l;
  arr[2].i;
  arr[2].l;
  arr[3].i;
  arr[3].l;

  return 0;
}