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

namespace align {
void enforceFieldAlign(const ASTContext &Context, FieldDecl *FD, unsigned Align);
void enforceStackAlign(const ASTContext &Context, VarDecl *VD, unsigned Align);
} // namespace align
} // namespace clang

#endif