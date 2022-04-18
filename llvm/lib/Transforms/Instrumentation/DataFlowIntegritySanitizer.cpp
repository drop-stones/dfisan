#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "UseDefAnalysis/UseDefAnalysisPass.h"
#include "UseDefAnalysis/UseDefChain.h"

using namespace llvm;
using namespace SVF;

const char DfisanModuleCtorName[] = "dfisan.module_ctor";
const char DfisanInitFunName[]    = "__dfisan_init";
const char DfisanStoreFunName[]   = "__dfisan_store_id";
const char DfisanLoadFunName[]    = "__dfisan_check_ids";

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  initializeSanitizerFuncs(M);

  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  SVF::UseDefChain *UseDef = Result.UseDef;

  IRBuilder<> Builder{M.getContext()};
  insertDfiInitFn(M, Builder);

  for (const auto *DefUsingPtr : UseDef->getDefUsingPtrList()) {
    insertDfiStoreFn(Builder, UseDef, DefUsingPtr);
  }
  for (const auto &Iter : *UseDef) {
    const LoadSVFGNode *Use = Iter.first;
    SmallVector<Value *, 8> DefIDs;
    for (const auto *Def : Iter.second) {
      Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(Def), false);
      DefIDs.push_back(DefID);
      insertDfiStoreFn(Builder, UseDef, Def);
    }
    insertDfiLoadFn(Builder, Use, DefIDs);
  }

  return PreservedAnalyses::all();
}

void DataFlowIntegritySanitizerPass::initializeSanitizerFuncs(Module &M) {
  LLVMContext &Ctx = M.getContext();

  VoidTy = Type::getVoidTy(Ctx);
  ArgTy  = Type::getInt16Ty(Ctx);
  PtrTy  = Type::getInt32PtrTy(Ctx);

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, ArgTy};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, ArgTy};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);

  DfiInitFn  = M.getOrInsertFunction(DfisanInitFunName, VoidTy);
  DfiStoreFn = M.getOrInsertFunction(DfisanStoreFunName, StoreFnTy);
  DfiLoadFn  = M.getOrInsertFunction(DfisanLoadFunName, LoadFnTy); // VarArg Function
}

/// Insert a constructor function in comdat
void DataFlowIntegritySanitizerPass::insertDfiInitFn(Module &M, IRBuilder<> &Builder) {
  // Create Sanitizer Ctor
  Function *Ctor = Function::createWithDefaultAttr(
    FunctionType::get(VoidTy, false),
    GlobalValue::InternalLinkage, 0, DfisanModuleCtorName, &M);
  Ctor->addFnAttr(Attribute::NoUnwind);
  BasicBlock *CtorBB = BasicBlock::Create(M.getContext(), "", Ctor);
  ReturnInst::Create(M.getContext(), CtorBB);

  // Insert Init Function
  Builder.SetInsertPoint(Ctor->getEntryBlock().getTerminator());
  Builder.CreateCall(DfiInitFn, {});

  // Ensure Ctor cannot be discarded, even if in a comdat.
  appendToUsed(M, {Ctor});
  // Put the constructor in GlobalCtors
  appendToGlobalCtors(M, Ctor, 1);
}

/// Insert a DEF function after each store statement using pointer.
void DataFlowIntegritySanitizerPass::insertDfiStoreFn(IRBuilder<> &Builder, UseDefChain *UseDef, const StoreSVFGNode *StoreNode) {
  // Check whether the insertion is first time.
  const Instruction *Inst = StoreNode->getInst();
  const Instruction *NextInst = Inst->getNextNode();
  if (const CallInst *Call = dyn_cast<const CallInst>(NextInst)) {
    const Function *Callee = Call->getCalledFunction();
    if (Callee == nullptr || Callee->getName() == DfisanStoreFunName)
      return;
  }

  if (StoreInst *Store = dyn_cast<StoreInst>((Instruction *)Inst)) {
    Value *StoreAddr = Store->getPointerOperand();
    Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(StoreNode), false);
    Builder.SetInsertPoint(Store->getNextNode());
    Builder.CreateCall(DfiStoreFn, {StoreAddr, DefID});
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(IRBuilder<> &Builder, const LoadSVFGNode *LoadNode, SmallVector<Value *, 8> &DefIDs) {
  // Check whether the insertion is first time.
  const Instruction *Inst = LoadNode->getInst();
  const Instruction *NextInst = Inst->getNextNode();
  if (const CallInst *Call = dyn_cast<const CallInst>(NextInst)) {
    const Function *Callee = Call->getCalledFunction();
    if (Callee == nullptr || Callee->getName() == DfisanLoadFunName)
      return;
  }

  if (LoadInst *Load = dyn_cast<LoadInst>((Instruction *)Inst)) {
    Value *LoadAddr = Load->getPointerOperand();
    Value *UseID = ConstantInt::get(ArgTy, LoadNode->getId(), false);
    Value *Argc  = ConstantInt::get(ArgTy, DefIDs.size(), false);
    SmallVector<Value *, 8> Args{LoadAddr, Argc};
    Args.append(DefIDs);
    Builder.SetInsertPoint(Load);
    Builder.CreateCall(DfiLoadFn, Args);
  }
}