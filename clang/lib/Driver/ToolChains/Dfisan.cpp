#include "Dfisan.h"
#include "clang/Driver/Compilation.h"

namespace clang {
namespace driver {
namespace toolchains {

using namespace tools;
using namespace llvm::opt;

Tool *DfisanToolChain::buildLinker() const {
  return new dfisan::Linker(*this);
}

void dfisan::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                  const InputInfo &Output,
                                  const InputInfoList &Inputs,
                                  const llvm::opt::ArgList &TCArgs,
                                  const char *LinkingOutput) const {
  llvm::errs() << "dfisan::Linker::" << __func__ << "\n";
  tools::gnutools::Linker::ConstructJob(C, JA, Output, Inputs, TCArgs, LinkingOutput);

  // Append "-T <linker script>" argument.
  if (!TCArgs.hasArg(options::OPT_T)) {
    // Get link command arguments.
    auto &Jobs = C.getJobs();
    auto &LinkJob = Jobs.getJobs().back();
    auto &CmdArgs = LinkJob->getArguments();

    // Copy the arguments.
    ArgStringList NewArgs = CmdArgs;

    // Append the linker script argument.
    // Assume that clang binary is in build/bin dir.
    const StringRef ClangDir = getToolChain().getDriver().getInstalledDir();
    const std::string LinkerScriptPath = std::string(ClangDir) + "/../../linker/linker_script.lds";
    NewArgs.push_back(TCArgs.MakeArgString("-T" + LinkerScriptPath));

    // Replace with the new arguments.
    LinkJob->replaceArguments(NewArgs);
  }
}

} // namespace toolchains
} // namespace driver
} // namespace clang
