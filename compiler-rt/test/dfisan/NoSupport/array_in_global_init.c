// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage array initialization in global.
// TODO: Support global variables.

struct S {
  int i;
  long l;
};

int arr0[8];            // uninitialized
int arr1[8] = { 100 };  // initialized with value
struct S arrS0[8];
struct S arrS1[8] = { { 200, 300 } };

int main(void) {
  arr0[0];
  arr0[1];
  arr0[4];
  arr0[7];

  arr1[0];
  arr1[1];
  arr1[4];
  arr1[7];

  arrS0[0].i;
  arrS0[0].l;
  arrS0[7].i;
  arrS0[7].l;

  arrS1[0].i;
  arrS1[0].l;
  arrS1[7].i;
  arrS1[7].l;

  return 0;
}
