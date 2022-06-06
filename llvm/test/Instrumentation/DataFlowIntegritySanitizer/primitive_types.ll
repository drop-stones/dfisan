; RUN: opt --passes=print-usedef %s

; ModuleID = '/home/shizuku/Development/dfisan/compiler-rt/test/dfisan/Support/primitive_types.c'
; source_filename = "/home/shizuku/Development/dfisan/compiler-rt/test/dfisan/Support/primitive_types.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 noundef %0, i8** noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i8, align 1
  %7 = alloca i8, align 1
  %8 = alloca i16, align 2
  %9 = alloca i16, align 2
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i64, align 8
  %13 = alloca i64, align 8
  %14 = alloca float, align 4
  %15 = alloca double, align 8
  %16 = alloca x86_fp80, align 16
  %17 = alloca fp128, align 16
  %18 = alloca x86_fp80, align 16
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  store i8 97, i8* %6, align 1
  store i8 98, i8* %7, align 1
  store i16 100, i16* %8, align 2
  store i16 200, i16* %9, align 2
  store i32 300, i32* %10, align 4
  store i32 400, i32* %11, align 4
  store i64 500, i64* %12, align 8
  store i64 600, i64* %13, align 8
  store float 0x3FF3AE1480000000, float* %14, align 4
  store double 4.560000e+00, double* %15, align 8
  store x86_fp80 0xK4001FC7AE147AE147800, x86_fp80* %16, align 16
  store fp128 0xL8000000000000000400243851EB851EB, fp128* %17, align 16
  %19 = load i8, i8* %6, align 1
  %20 = sext i8 %19 to i32
  %21 = load i8, i8* %7, align 1
  %22 = zext i8 %21 to i32
  %23 = add nsw i32 %20, %22
  %24 = load i16, i16* %8, align 2
  %25 = sext i16 %24 to i32
  %26 = add nsw i32 %23, %25
  %27 = load i16, i16* %9, align 2
  %28 = zext i16 %27 to i32
  %29 = add nsw i32 %26, %28
  %30 = load i32, i32* %10, align 4
  %31 = add nsw i32 %29, %30
  %32 = load i32, i32* %11, align 4
  %33 = add i32 %31, %32
  %34 = zext i32 %33 to i64
  %35 = load i64, i64* %12, align 8
  %36 = add nsw i64 %34, %35
  %37 = load i64, i64* %13, align 8
  %38 = add i64 %36, %37
  %39 = uitofp i64 %38 to float
  %40 = load float, float* %14, align 4
  %41 = fadd float %39, %40
  %42 = fpext float %41 to double
  %43 = load double, double* %15, align 8
  %44 = fadd double %42, %43
  %45 = fpext double %44 to x86_fp80
  %46 = load x86_fp80, x86_fp80* %16, align 16
  %47 = fadd x86_fp80 %45, %46
  %48 = fpext x86_fp80 %47 to fp128
  %49 = load fp128, fp128* %17, align 16
  %50 = fadd fp128 %48, %49
  %51 = fptrunc fp128 %50 to x86_fp80
  store x86_fp80 %51, x86_fp80* %18, align 16
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 1}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 329fda39c507e8740978d10458451dcdb21563be)"}
