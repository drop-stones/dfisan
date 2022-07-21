// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage casts of struct in function arguments.

struct Vec2D {
  int x;
  int y;
};

// struct Vec2D -> i64
void readVec2D(struct Vec2D v) {
  v.x;  // OK
  v.y;  // OK
}

int main(void) {
  struct Vec2D v = { 100, 200 };

  v.x;  // OK
  v.y;

  readVec2D(v);

  return 0;
}