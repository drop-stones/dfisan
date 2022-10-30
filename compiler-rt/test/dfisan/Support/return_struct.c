// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage a return value of struct.
//   - move_s: compiled into pointer args
//   - Vec2D : compiled into i64

struct move_s {
  int from;
  int target;
  int captured;
  int promoted;
  int castled;
  int ep;
};

// IR: void @createMoveS(%struct.move_s *);
//   - Pointer args
struct move_s createMoveS() {
  struct move_s new_m = { 100, 200, 300, 400, 500, 600 };
  return new_m;
}

struct Vec2D {
  int x, y;
};

// IR: i64 @createVec2D()
struct Vec2D createVec2D() {
  struct Vec2D new_vec = { 100, 200 };
  return new_vec;
}

int main(void) {
  struct move_s m __attribute__((annotate("dfi_protection")));
  m = createMoveS();

  m.from;
  m.target;
  m.captured;
  m.promoted;
  m.castled;
  m.ep;

  struct Vec2D v __attribute__((annotate("dfi_protection")));
  v = createVec2D();

  v.x; v.y;

  return 0;
}
