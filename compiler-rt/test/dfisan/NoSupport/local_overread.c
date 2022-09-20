// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect overread of local variable.

int main(int argc, char **argv) {
  int x = 100;
  int arr[8];
  for (int i = 0; i < 8; i++)
    arr[i] = i;
  
  x;          // OK!
  arr[12];    // Error: read 'x'
  return 0;
}