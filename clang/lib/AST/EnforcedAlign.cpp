#include "clang/AST/EnforcedAlign.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "llvm/Support/raw_ostream.h"

namespace {
using namespace clang;

bool isSmallerThanAlign(const ASTContext &Context, DeclaratorDecl *D, unsigned Align) {
  const QualType Ty = D->getType();
  unsigned TyAlign = Context.getTypeAlign(Ty) / 8;  // Bits to Bytes
  if (TyAlign < Align)
    return true;
  return false;
}

void AddAlignAttr(DeclaratorDecl *D, unsigned Align) {
  llvm::outs() << __func__ << ": " << D->getDeclName() << "\n";
}
} // anonymous namespace

namespace clang {
namespace align {
void enforceFieldAlign(const ASTContext &Context, FieldDecl *FD, unsigned Align) {
  llvm::outs() << __func__ << ": " << FD->getDeclName() << "\n";

  if (isSmallerThanAlign(Context, FD, Align))
    AddAlignAttr(FD, Align);
}

void enforceStackAlign(const ASTContext &Context, VarDecl *VD, unsigned Align) {
  if (!VD->isLocalVarDeclOrParm())
    return;
  
  llvm::outs() << __func__ << ": " << VD->getDeclName() << "\n";

  if (isSmallerThanAlign(Context, VD, Align))
    AddAlignAttr(VD, Align);
}
} // namespace align
} // namespace clang