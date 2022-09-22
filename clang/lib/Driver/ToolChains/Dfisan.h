#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DFISAN_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DFISAN_H

#include "llvm/Support/Compiler.h"
#include "Linux.h"

namespace clang {
namespace driver {
namespace toolchains {

struct LLVM_LIBRARY_VISIBILITY DfisanToolChain : public Linux {
  DfisanToolChain(const Driver &D, const llvm::Triple &Triple, const llvm::opt::ArgList &Args)
    : Linux(D, Triple, Args) {}

protected:
  Tool *buildAssembler() const override;
  Tool *buildLinker() const override;
};

namespace dfisan {

// Not used yet (used Clang::ConstructJob()).
struct LLVM_LIBRARY_VISIBILITY Assembler : public tools::gnutools::Assembler {
  Assembler(const ToolChain &TC) : tools::gnutools::Assembler(TC) {}

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output,
                    const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

struct LLVM_LIBRARY_VISIBILITY Linker : public tools::gnutools::Linker {
  Linker(const ToolChain &TC) : tools::gnutools::Linker(TC) {}

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output,
                    const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};
} // namespace dfisan

} // namespace toolchains
} // namespace driver
} // namespace clang

#endif
