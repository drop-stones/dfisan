#ifndef DG_DFI_OPTIONS_H_
#define DG_DFI_OPTIONS_H_

#include "dg/AnalysisOptions.h"
#include "dg/llvm/DataDependence/LLVMDataDependenceAnalysisOptions.h"
#include "llvm/Support/raw_ostream.h"

namespace dg {

const unsigned RETURN = Offset::getUnknown().offset;
const unsigned VARARG = RETURN - 1;

struct DfiLLVMDataDependenceAnalysisOptions : public LLVMDataDependenceAnalysisOptions {
  DfiLLVMDataDependenceAnalysisOptions()
    : LLVMDataDependenceAnalysisOptions() {
    // Setup additional models for stdlib functions.
    // FunctionModel: (<func name>, {<operand>, <beg offset>, <size operand>})
    //  Vararg Model: (<func name>, { VARARG, <beg operand>, Offset::getUnknown()})

    // Memory block functions
    functionModelAddDef("llvm.memset.p0i8.i64", {0, Offset(0), 2});

    // IO functions
    functionModelAddDef("read", {1, Offset(0), 2});
    functionModelAddDef("fgets", {0, Offset(0), 1});
    functionModelAddDef("__isoc99_sscanf", {VARARG, 2, Offset::getUnknown()});
  }
};

} // namespace dg

#endif
