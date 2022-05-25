// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in local struct.

struct Array {
  char arr[8];
  int id;
};

int main(int argc, char **argv) {
  struct Array Arr;
  Arr.id = 100;
  for (int i = 0; i < 9; i++)
    Arr.arr[i] = 'a';

  Arr.id;   // Error: read broken 'Arr.id'
  return 0;
}