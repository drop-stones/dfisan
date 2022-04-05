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
        -DBUILD_SHARED_LIBS=ON            \
        -DLLVM_USE_SPLIT_DWARF=ON         \
        -DLLVM_OPTIMIZED_TABLEGEN=ON      \
        -DCMAKE_INSTALL_RPATH="./lib"     \
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
        -DCMAKE_C_COMPILER=clang          \
        -DCMAKE_CXX_COMPILER=clang++      \
        -DLLVM_USE_NEWPM=ON               \
        -DLLVM_ENABLE_Z3_SOLVER=ON        \
        ../llvm
$ ninja clang compiler-rt opt llvm-profdata
```

## Run

```
$ bin/opt -passes=print-field-sensitive-pta -disable-output /path/to/file
```

## Testing

### Unittest

```
$ ninja check-llvm-unit
```

### Regression Test

```
$ ninja check-llvm
```
