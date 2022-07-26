// RUN: %clang_dfisan %s -o %t
// RUN: %run %t %s
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can support fgets().

#include <stdio.h>
#include <assert.h>

#define STR_BUFF 256

int main(int argc, char **argv) {
  char readbuff[STR_BUFF];

  FILE *file = fopen(argv[1], "r");
  while (fgets(readbuff, STR_BUFF, file) != NULL) {
    readbuff[0];
    readbuff[1];
    printf("%s", readbuff);
  }

  return 0;
}
