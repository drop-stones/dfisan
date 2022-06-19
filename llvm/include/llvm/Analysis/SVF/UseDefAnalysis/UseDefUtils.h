
#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFUTILS_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFUTILS_H

#include "Util/SVFModule.h"

using namespace llvm;
using namespace SVF;

std::string getModuleName(Module &M);
std::string getModuleName(SVFModule *M);

#endif