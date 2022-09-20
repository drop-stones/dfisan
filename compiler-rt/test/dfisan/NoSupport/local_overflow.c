// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow of local variable.

int main(int argc, char **argv) {
  int x = 100;
  int arr[8];
  for (int i = 0; i < 12; i++)
    arr[i] = i;

  x;  // Error: read broken 'x'
  return 0;
}