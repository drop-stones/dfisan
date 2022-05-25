#include "clang/AST/EnforcedAlign.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "clang/Sema/Sema.h"
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

void AddAlignAttr(Sema &S, DeclaratorDecl *D, unsigned Align) {
  llvm::outs() << __func__ << ": " << D->getDeclName() << "\n";

  ASTContext &Context = S.getASTContext();
  llvm::APInt AlignNum{32, 4};
  llvm::APSInt SignedAlignNum{AlignNum};
  clang::APValue AlignNumValue{SignedAlignNum};
  QualType Qual{Context.getIntTypeForBitwidth(32, 1)};
  IntegerLiteral *IL = IntegerLiteral::Create(Context, AlignNum, Qual, SourceLocation());
  ConstantExpr *CE = ConstantExpr::Create(Context, IL, AlignNumValue);
  AlignedAttr *AlignAttr = AlignedAttr::Create(
    Context, /* IsAlignmentExpr */ true,
    CE, {},
    AttributeCommonInfo::AS_GNU, AlignedAttr::GNU_aligned
  );
  D->addAttr(AlignAttr);
  //S.AddAlignedAttr(D);
}
} // anonymous namespace

namespace clang {
namespace align {
void enforceFieldAlign(Sema &S, FieldDecl *FD, unsigned Align) {
  llvm::outs() << __func__ << ": " << FD->getDeclName() << "\n";

  if (isSmallerThanAlign(S.getASTContext(), FD, Align))
    AddAlignAttr(S, FD, Align);
}

void enforceStackAlign(Sema &S, VarDecl *VD, unsigned Align) {
  if (!VD->isLocalVarDeclOrParm())
    return;
  
  llvm::outs() << __func__ << ": " << VD->getDeclName() << "\n";

  if (isSmallerThanAlign(S.getASTContext(), VD, Align))
    AddAlignAttr(S, VD, Align);
}

void enforceGlobalAlign(Sema &S, VarDecl *VD, unsigned Align) {
  if (!VD->hasGlobalStorage())
    return;

  llvm::outs() << __func__ << ": " << VD->getDeclName() << "\n";

  if (isSmallerThanAlign(S.getASTContext(), VD, Align))
    AddAlignAttr(S, VD, Align);
}
} // namespace align
} // namespace clang