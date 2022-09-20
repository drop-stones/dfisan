// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage byval in function arguments.

struct move_s {
  int from;
  int target;
  int captured;
  int promoted;
  int castled;
  int ep;
};

void readMoveS(struct move_s m) {
  m.from;
  m.target;
  m.captured;
  m.promoted;
  m.castled;
  m.ep;
}

int main(void) {
  struct move_s m = { 100, 200, 300, 400, 500, 600 };

  m.from;     // OK
  m.target;
  m.captured;
  m.promoted;
  m.castled;
  m.ep;

  readMoveS(m);

  return 0;
}