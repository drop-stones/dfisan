// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage a return value of struct.

struct move_s {
  int from;
  int target;
  int captured;
  int promoted;
  int castled;
  int ep;
};

struct move_s createMoveS() {
  struct move_s new_m = { 100, 200, 300, 400, 500, 600 };
  return new_m;
}

int main(void) {
  struct move_s m = createMoveS();

  m.from;
  m.target;
  m.captured;
  m.promoted;
  m.castled;
  m.ep;

  return 0;
}
