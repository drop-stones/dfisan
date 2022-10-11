// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect overread in local struct.

struct Array {
  char arr[8];
  int id;
};

int main(int argc, char **argv) {
  struct Array Arr __attribute__((annotate("dfi_protection")));
  Arr.id = 100;
  for (int i = 0; i < 8; i++)
    Arr.arr[i] = 'a';
  
  Arr.id;       // OK!
  Arr.arr[9];   // Error: read 'Arr.id'
  return 0;
}