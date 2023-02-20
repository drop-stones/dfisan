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

No instrument unsafe accesses
```
$ bin/clang -fsanitize=dfi -mllvm -no-check-unsafe-access foo.c
```

Protect all data and detect memory errors only
```
$ bin/clang -fsanitize=dfi -mllvm -protect-all -mllvm -no-check-unsafe-access -mllvm -no-error-report -mllvm data-race-detection=false
```

Instrument checks + sets with function call (default: instrument with LLVM IR)  
And it prints dynamic statistics on the number of checks + sets.
```
$ bin/clang -fsanitize=dfi -mllvm -check-with-call foo.c
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
