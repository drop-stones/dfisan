// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage big multiple array.
// TODO: Support memset() of array.

#define ARR_SIZE 144

int garr[ARR_SIZE][ARR_SIZE];

int main(void) {
  int larr[ARR_SIZE][ARR_SIZE] = { 0 };

  for (int i = 0; i < ARR_SIZE; i++)
    for (int j = 0; j < ARR_SIZE; j++) {
      garr[i][j];
      larr[i][j];
    }
  
  return 0;
}
