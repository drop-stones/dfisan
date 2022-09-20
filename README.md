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
