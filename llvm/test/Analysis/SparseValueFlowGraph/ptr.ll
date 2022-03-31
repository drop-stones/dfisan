; RUN: opt %s -passes=dump-svfg -disable-output 2>&1

define dso_local void @ptr_test() #0 {
  %1 = alloca i32*, align 8
  %2 = alloca i32*, align 8
  %3 = alloca i32, align 4
  store i32 100, i32* %3, align 4
  store i32* %3, i32** %1, align 8
  store i32* %3, i32** %2, align 8
  ret void
}
