#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "UseDefAnalysis/UseDefAnalysisPass.h"
#include "UseDefAnalysis/UseDefChain.h"

using namespace llvm;
using namespace SVF;

constexpr char DfisanModuleCtorName[] = "dfisan.module_ctor";
constexpr char DfisanInitFnName[]     = "__dfisan_init";
constexpr char CommonDfisanStoreFnName[] = "__dfisan_store_id";
constexpr char CommonDfisanLoadFnName[]  = "__dfisan_check_ids";
constexpr char DfisanStoreNFnName[]    = "__dfisan_store_id_n";
constexpr char DfisanLoadNFnName[]     = "__dfisan_check_ids_n";

constexpr char DfisanStore1FnName[]   = "__dfisan_store_id_1";
constexpr char DfisanStore2FnName[]   = "__dfisan_store_id_2";
constexpr char DfisanStore4FnName[]   = "__dfisan_store_id_4";
constexpr char DfisanStore8FnName[]   = "__dfisan_store_id_8";
constexpr char DfisanStore16FnName[]  = "__dfisan_store_id_16";

constexpr char DfisanLoad1FnName[]    = "__dfisan_check_ids_1";
constexpr char DfisanLoad2FnName[]    = "__dfisan_check_ids_2";
constexpr char DfisanLoad4FnName[]    = "__dfisan_check_ids_4";
constexpr char DfisanLoad8FnName[]    = "__dfisan_check_ids_8";
constexpr char DfisanLoad16FnName[]   = "__dfisan_check_ids_16";

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  initializeSanitizerFuncs(M);

  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  Svfg = Result.Svfg;
  UseDef = Result.UseDef;

  IRBuilder<> Builder{M.getContext()};
  insertDfiInitFn(M, Builder);

  for (const auto *DefUsingPtr : UseDef->getDefUsingPtrList()) {
    insertDfiStoreFn(M, Builder, DefUsingPtr);
  }
  for (const auto &Iter : *UseDef) {
    const LoadSVFGNode *Use = Iter.first;
    SmallVector<Value *, 8> DefIDs;
    for (const auto *Def : Iter.second) {
      Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(Def), false);
      DefIDs.push_back(DefID);
      insertDfiStoreFn(M, Builder, Def);
    }
    insertDfiLoadFn(M, Builder, Use, DefIDs);
  }

  return PreservedAnalyses::all();
}

void DataFlowIntegritySanitizerPass::initializeSanitizerFuncs(Module &M) {
  LLVMContext &Ctx = M.getContext();

  int LongSize = M.getDataLayout().getPointerSizeInBits();
  PtrTy  = Type::getIntNTy(Ctx, LongSize);
  VoidTy = Type::getVoidTy(Ctx);
  ArgTy  = Type::getInt16Ty(Ctx);

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, ArgTy};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, ArgTy};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);

  DfiInitFn  = M.getOrInsertFunction(DfisanInitFnName, VoidTy);
  DfiStoreNFn = M.getOrInsertFunction(DfisanStoreNFnName, StoreFnTy);
  DfiLoadNFn  = M.getOrInsertFunction(DfisanLoadNFnName, LoadFnTy); // VarArg Function

  DfiStore1Fn = M.getOrInsertFunction(DfisanStore1FnName, StoreFnTy);
  DfiStore2Fn = M.getOrInsertFunction(DfisanStore2FnName, StoreFnTy);
  DfiStore4Fn = M.getOrInsertFunction(DfisanStore4FnName, StoreFnTy);
  DfiStore8Fn = M.getOrInsertFunction(DfisanStore8FnName, StoreFnTy);
  DfiStore16Fn = M.getOrInsertFunction(DfisanStore16FnName, StoreFnTy);

  DfiLoad1Fn = M.getOrInsertFunction(DfisanLoad1FnName, LoadFnTy);
  DfiLoad2Fn = M.getOrInsertFunction(DfisanLoad2FnName, LoadFnTy);
  DfiLoad4Fn = M.getOrInsertFunction(DfisanLoad4FnName, LoadFnTy);
  DfiLoad8Fn = M.getOrInsertFunction(DfisanLoad8FnName, LoadFnTy);
  DfiLoad16Fn = M.getOrInsertFunction(DfisanLoad16FnName, LoadFnTy);
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

  // Insert DfiStoreFn for GlobalInit
  for (const auto &GlobalInit : UseDef->getGlobalInitList()) {
    const Value *Val = GlobalInit->getValue();
    assert(Val != nullptr);
    if (const auto *ConstData = dyn_cast<const ConstantData>(Val)) {
      const auto DefVars = GlobalInit->getDefSVFVars();
      for (const auto DefVarID : DefVars) {
        const auto *DstNode = Svfg->getPAG()->getObject(DefVarID);
        Value *StorePointer = (Value *)DstNode->getValue();
        createDfiStoreFn(M, Builder, GlobalInit, StorePointer, Builder.GetInsertBlock()->getTerminator());
      }
    }
  }
}

/// Insert a DEF function after each store statement using pointer.
void DataFlowIntegritySanitizerPass::insertDfiStoreFn(Module &M, IRBuilder<> &Builder, const StoreSVFGNode *StoreNode) {
  // Check whether the insertion is first time.
  const Instruction *Inst = StoreNode->getInst();
  if (Inst == nullptr)
    return;

  const Instruction *NextToTwoInst = (Inst->getNextNode() != nullptr) ? Inst->getNextNode()->getNextNode() : nullptr;
  if (NextToTwoInst != nullptr) {
    if (const CallInst *Call = dyn_cast<const CallInst>(NextToTwoInst)) {
      const Function *Callee = Call->getCalledFunction();
      if (Callee == nullptr || Callee->getName().contains(CommonDfisanStoreFnName))
        return;
    }
  }

  if (StoreInst *Store = dyn_cast<StoreInst>((Instruction *)Inst)) {
    createDfiStoreFn(M, Builder, StoreNode, Store->getPointerOperand(), Store->getNextNode());
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(Module &M, IRBuilder<> &Builder, const LoadSVFGNode *LoadNode, SmallVector<Value *, 8> &DefIDs) {
  // Check whether the insertion is first time.
  const Instruction *Inst = LoadNode->getInst();
  const Instruction *PrevInst = Inst->getPrevNode();  // error occured
  if (PrevInst != nullptr) {
    if (const CallInst *Call = dyn_cast<const CallInst>(PrevInst)) {
      const Function *Callee = Call->getCalledFunction();
      if (Callee == nullptr || Callee->getName().contains(CommonDfisanLoadFnName))
        return;
    }
  }

  if (LoadInst *Load = dyn_cast<LoadInst>((Instruction *)Inst)) {
    createDfiLoadFn(M, Builder, LoadNode, Load->getPointerOperand(), Load, DefIDs);
  }
}

/// Create a function call to DfiStoreFn.
void DataFlowIntegritySanitizerPass::createDfiStoreFn(Module &M, IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode, Value *StorePointer, Instruction *InsertPoint) {
  Builder.SetInsertPoint(InsertPoint);

  Type *StoreTy = StorePointer->getType()->getNonOpaquePointerElementType();
  unsigned StoreSize = M.getDataLayout().getTypeStoreSize(StoreTy);
  Value *StoreSizeVal = ConstantInt::get(ArgTy, StoreSize, false);
  Value *StoreAddr = Builder.CreatePtrToInt(StorePointer, PtrTy);
  Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(StoreNode), false);

  switch(StoreSize) {
  case 1:   Builder.CreateCall(DfiStore1Fn, {StoreAddr, DefID});  break;
  case 2:   Builder.CreateCall(DfiStore2Fn, {StoreAddr, DefID});  break;
  case 4:   Builder.CreateCall(DfiStore4Fn, {StoreAddr, DefID});  break;
  case 8:   Builder.CreateCall(DfiStore8Fn, {StoreAddr, DefID});  break;
  case 16:  Builder.CreateCall(DfiStore16Fn,{StoreAddr, DefID});  break;
  default:  Builder.CreateCall(DfiStoreNFn, {StoreAddr, StoreSizeVal, DefID});  break;
  }
}

/// Create a function call to DfiLoadFn.
void DataFlowIntegritySanitizerPass::createDfiLoadFn(Module &M, IRBuilder<> &Builder, const SVF::LoadVFGNode *LoadNode, Value *LoadPointer, Instruction *InsertPoint, SmallVector<Value *, 8> &DefIDs) {
  Builder.SetInsertPoint(InsertPoint);

  Type  *LoadTy = LoadPointer->getType()->getNonOpaquePointerElementType();
  unsigned LoadSize = M.getDataLayout().getTypeStoreSize(LoadTy);
  Value *LoadSizeVal = ConstantInt::get(ArgTy, LoadSize, false);
  Value *LoadAddr = Builder.CreatePtrToInt(LoadPointer, PtrTy); // Cast 'i32 **' to 'i64'
  //Value *UseID = ConstantInt::get(ArgTy, LoadNode->getId(), false);
  Value *Argc  = ConstantInt::get(ArgTy, DefIDs.size(), false);

  SmallVector<Value *, 8> Args{LoadAddr, Argc};
  Args.append(DefIDs);

  switch(LoadSize) {
  case 1:   Builder.CreateCall(DfiLoad1Fn, Args);   break;
  case 2:   Builder.CreateCall(DfiLoad2Fn, Args);   break;
  case 4:   Builder.CreateCall(DfiLoad4Fn, Args);   break;
  case 8:   Builder.CreateCall(DfiLoad8Fn, Args);   break;
  case 16:  Builder.CreateCall(DfiLoad16Fn,Args);   break;
  default:  auto *Iter = Args.begin() + 1;
            Args.insert(Iter, LoadSizeVal);
            Builder.CreateCall(DfiLoadNFn, Args);   break;
  }
}