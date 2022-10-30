# DFISAN

## Description

TODO

## Compilation

Required dependencies: LLVM-14, lit + FileCheck

```
$ git clone https://github.com/drop-stones/dfisan.git
$ cd dfisan
$ mkdir build && cd build
$ cmake -G Ninja ../llvm
$ ninja
```

## Usage

### Compile C Code with dfisan Instrumentation

```
$ bin/clang -fsanitize=dfi foo.c
```

#### Some options

Output def-use to sqlite3
```
$ bin/clang -fsanitize=dfi -mllvm --debug-only=usedef-log foo.c
```

Use a single unaligned region only
```
$ bin/clang -fsanitize=dfi -mllvm -unaligned-region-only foo.c
```

Instrument all unsafe accesses (default: instrument only variable-offset accesses)
```
$ bin/clang -fsanitize=dfi -mllvm -check-all-unsafe-access foo.c
```

### Testing

```
$ ninja check-dfisan
```

### Debugging

#### ReplaceWithSafeAllocPass

```
$ bin/opt --passes=replace-with-safe-alloc --debug-only=protection-target,replace-with-safe-alloc --disable-output foo.ll
```

#### UseDefAnalysisPass

```
$ bin/opt --passes=print-usedef --debug-only=<DEBUG_TYPE> --disable-output foo.ll
```

#### DataFlowIntegritySanitizerPass

```
$ bin/opt --passes=dfisan --debug-only=<DEBUG_TYPE> --disable-output foo.ll
```
