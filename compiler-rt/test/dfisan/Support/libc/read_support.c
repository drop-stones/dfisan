// RUN: %clang_dfisan %s -o %t
// RUN: %run %t %s
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage read().

#include <unistd.h>
#include <fcntl.h>

#define STR_BUFF 256

int main(int argc, char **argv) {
  char readbuff[STR_BUFF] __attribute__((annotate("dfi_protection")));

  int fd, rc, i;
  fd = open(argv[1], O_RDONLY);
  while (1) {
    rc = read(fd, readbuff, STR_BUFF);  // DEF: readbuff[0-rc]
    if (rc == 0)
      break;
    for (i = 0; i < rc; i++)
      readbuff[i];                      // USE: readbuff[0-rc]
  }

  return 0;
}
