// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage a return value of struct with cast.

struct Vec2D {
  int x;
  int y;
};

// struct Vec2D -> i64
struct Vec2D createVec2D() {
  struct Vec2D new_vec = { 100, 200 };
  return new_vec;
}

int main(void) {
  struct Vec2D v = createVec2D();

  v.x;
  v.y;

  return 0;
}
