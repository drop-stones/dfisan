#ifndef LLVM_CLANG_AST_ENFORCEDALIGN_H
#define LLVM_CLANG_AST_ENFORCEDALIGN_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Demangle/ItaniumDemangle.h"

namespace llvm {
template <typename T> class SmallVectorImpl;
} // namespace llvm

namespace clang {

class ASTContext;
class FieldDecl;
class VarDecl;
class Sema;

namespace align {
void enforceFieldAlign(Sema &S, FieldDecl *FD, unsigned Align);
void enforceStackAlign(Sema &S, VarDecl *VD, unsigned Align);
} // namespace align
} // namespace clang

#endif