# LLVM-14 skeleton

## Description

This project is a llvm-14 skeleton for additional implementation.

## Compilation

```
$ cmake -G Ninja \
	-DLLVM_ENABLE_PROJECTS="clang;compiler-rt" \
	-DLLVM_USE_LINKER=lld             \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DLLVM_TARGETS_TO_BUILD=X86       \
	-DLLVM_BUILD_LLVM_DYLIB=ON        \
	-DLLVM_LINK_LLVM_DYLIB=ON         \
	-DLLVM_USE_SPLIT_DWARF=ON         \
	-DLLVM_OPTIMIZED_TABLEGEN=ON      \
	-DCMAKE_C_COMPILER=clang          \
	-DCMAKE_CXX_COMPILER=clang++      \
	-DLLVM_USE_NEWPM=ON               \
	../llvm
$ ninja clang compiler-rt opt llvm-profdata
```
