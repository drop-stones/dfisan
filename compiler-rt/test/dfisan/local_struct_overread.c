// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect overread in local struct.

struct Array {
  char arr[8];
  int id;
};

int main(int argc, char **argv) {
  struct Array Arr;
  Arr.id = 100;
  for (int i = 0; i < 8; i++)
    Arr.arr[i] = 'a';
  
  Arr.id;       // OK!
  Arr.arr[9];   // Error: read 'Arr.id'
  return 0;
}