//
// <legal>
// Pointer Ownership Model (POM) Source Code Release
// 
// Copyright 2025 Carnegie Mellon University.
// 
// NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
// INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
// UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
// IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
// FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
// OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
// MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
// TRADEMARK, OR COPYRIGHT INFRINGEMENT.
// 
// Licensed under a MIT (SEI)-style license, please see license.txt or
// contact permission@sei.cmu.edu for full terms.
// 
// [DISTRIBUTION STATEMENT A] This material has been approved for public
// release and unlimited distribution.  Please see Copyright notice for
// non-US Government use and distribution.
// 
// DM25-1262
// </legal>
//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/ADT/SmallVector.h"
#include <map>
#include <vector>


/*****************************************************************************
* This pass must run *before* the mem2reg pass.
* This pass (1) declares a dummy function `__pom_var_store` and (2)
* identifies stores to local variables of pointer type and changes them as
* described below.  Only local variables of pointer type are be affected.
* 
* Example input:
*   ```
*   %dest_local_var = alloca ptr, align 8
*   ...
*   call void @llvm.dbg.declare(metadata ptr %dest_local_var, metadata !39, metadata !DIExpression())
*   ...
*   store ptr %val_to_store, ptr %dest_local_var
*   ...
*   !39 = !DILocalVariable(name: "q1", scope: !17, file: !2, line: 7, type: !29)
*   ```
* 
* For the above example, this LLVM pass changes
*   ```
*   store ptr %val_to_store, ptr %dest_local_var
*   ```
* to
*   ```
*   %temp = call ptr @__pom_var_store(ptr %val_to_store), !pom_orig_var !39
*   store ptr %temp, ptr %dest_local_var
*   ```
* Here, the `!pom_orig_var !39` metadata comes from the
* `call void @llvm.dbg.declare(metadata ptr %dest_local_var,  metadata !39, ...)`
* instruction.
*****************************************************************************/

using namespace llvm;

class VarStorePassImpl {
private:
    Function *pomVarStoreFunc = nullptr;
    std::map<Value*, DILocalVariable*> allocaToDebugInfo;

public:
    bool runOnFunction(Function &F);
    
private:
    void declareVarStoreFunction(Module &M);
    void findAllocaDebugInfo(Function &F);
    bool transformStores(Function &F);

    void fixupNoArgCalls(Function& F) {
        // Declarations like `int foo();` (in contrast to `int foo(void);`) result in a mess;
        // we clean it up here.
        for (llvm::BasicBlock &BB : F) {
            for (auto It = BB.begin(), E = BB.end(); It != E; ) {
                llvm::Instruction *I = &*It++;
                auto *CB = llvm::dyn_cast<llvm::CallBase>(I);
                if (!CB) continue;

                // Try to get a direct function symbol.
                llvm::Value *CalleeOp = CB->getCalledOperand();
                llvm::Value *Stripped = CalleeOp->stripPointerCasts();
                auto *CalleeFn = llvm::dyn_cast<llvm::Function>(Stripped);
                if (!CalleeFn) continue;

                // We want a prototype like `i32 ()` and a call with no args.
                auto *FTy = CalleeFn->getFunctionType();
                if (FTy->isVarArg() || FTy->getNumParams() != 0) continue;
                if (CB->arg_size() != 0) continue;

                // If the call already matches, skip.
                if (CB->getFunctionType() == FTy && CB->getCalledOperand() == CalleeFn) {
                    continue;
                }

                // Rebuild the call so its callee is the function itself (no cast).
                llvm::IRBuilder<> B(CB);
                auto *NewCall = B.CreateCall(CalleeFn, {});
                NewCall->setCallingConv(CB->getCallingConv());
                NewCall->setTailCall(CB->isTailCall());
                NewCall->setAttributes(CB->getAttributes());
                if (CB->isTailCall() && CB->isMustTailCall()) {
                    NewCall->setTailCallKind(llvm::CallInst::TCK_MustTail);
                }

                CB->replaceAllUsesWith(NewCall);
                CB->eraseFromParent();
            }
        }
    }

};

void VarStorePassImpl::declareVarStoreFunction(Module &M) {
    if (pomVarStoreFunc) return;
    
    LLVMContext &Context = M.getContext();
    Type *PtrType = PointerType::getUnqual(Context);
    
    FunctionType *FT = FunctionType::get(PtrType, {PtrType}, false);
    pomVarStoreFunc = Function::Create(FT, Function::ExternalLinkage, "__pom_var_store", &M);
}

void VarStorePassImpl::findAllocaDebugInfo(Function &F) {
    allocaToDebugInfo.clear();
    
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (auto *DII = dyn_cast<DbgDeclareInst>(&I)) {
                if (auto *Alloca = dyn_cast_or_null<AllocaInst>(DII->getAddress())) {
                    if (auto *DIVar = DII->getVariable()) {
                        allocaToDebugInfo[Alloca] = DIVar;
                    }
                }
            }
        }
    }
}

bool VarStorePassImpl::transformStores(Function &F) {
    std::vector<StoreInst*> StoresToTransform;
    std::vector<ReturnInst*> returnInsts;
    LLVMContext& Ctx = F.getParent()->getContext();
    
    // Find stores to local pointer variables
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (auto *SI = dyn_cast<StoreInst>(&I)) {
                // Check if storing a pointer value
                if (!SI->getValueOperand()->getType()->isPointerTy()) {
                    continue;
                }
                std::vector<Metadata*> unsupported_features;
                Value* ptr = SI->getPointerOperand();
                if (isa<GlobalVariable>(ptr)) {
                    unsupported_features.push_back(MDString::get(Ctx, "global_ptr_write"));
                }
                if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(ptr)) {
                    Type* sourceElementType = GEP->getSourceElementType();
                    if (isa<StructType>(sourceElementType)) {
                        unsupported_features.push_back(MDString::get(Ctx, "write_ptr_to_struct"));
                    }
                }
                MDNode *MD = MDNode::get(Ctx, unsupported_features);
                SI->setMetadata("unsupported_features", MD);
                                
                // Check if storing to a local variable (alloca)
                if (auto *Alloca = dyn_cast<AllocaInst>(SI->getPointerOperand())) {
                    // Check if alloca is for pointer type
                    if (Alloca->getAllocatedType()->isPointerTy()) {
                        StoresToTransform.push_back(SI);
                    }
                }
            } else if (auto *RI = dyn_cast<ReturnInst>(&I)) {
                returnInsts.push_back(RI);
            }
        }
    }

    // Clang with "-O0" produces functions with exactly 1 return instruction.
    Value* retval_addr;
    if (returnInsts.size() != 1) {
        errs() << "Error in function " << F.getName().str() << ": found " <<
            returnInsts.size() << " return instructions (expecting exactly 1).\n";
    } else {
        Value *retVal = returnInsts[0]->getReturnValue();
        if (retVal) {
            if (LoadInst* load = dyn_cast<LoadInst>(retVal)) {
                retval_addr = load->getPointerOperand();
            }
        }
    }
    
    // Transform the stores
    for (auto *SI : StoresToTransform) {
        IRBuilder<> Builder(SI);
        DILocalVariable *DIVar = nullptr;
        
        // Try to find metadata
        if (auto *Alloca = dyn_cast<AllocaInst>(SI->getPointerOperand())) {
            auto It = allocaToDebugInfo.find(Alloca);
            if (It != allocaToDebugInfo.end()) {
                DIVar = It->second;
            }
        }

        if (SI->getParent()->isEntryBlock() && isa<Argument>(SI->getValueOperand())) {
            continue;
        }
        
        // Create call to __pom_var_store
        Value *StoredValue = SI->getValueOperand();
        CallInst *Call = Builder.CreateCall(pomVarStoreFunc, {StoredValue});

        // Add metadata if we have it
        if (DIVar) {
            Call->setMetadata("pom_orig_var", DIVar);
        } else if (SI->getPointerOperand() == retval_addr) {
          LLVMContext &ctx = SI->getContext();
          auto md = dyn_cast<ConstantAsMetadata>(llvm::ValueAsMetadata::getConstant(
            ConstantInt::get(Type::getInt64Ty(ctx), 1)));
          MDNode *md_node =  MDTuple::get(ctx, md);
          Call->setMetadata("is_retval", md_node);
        }
        
        // Create new store with the call result
        Builder.CreateStore(Call, SI->getPointerOperand());
        
        // Remove the old store
        SI->eraseFromParent();
    }
    
    bool Modified = true;
    return Modified;
}

bool VarStorePassImpl::runOnFunction(Function &F) {
    Module &M = *F.getParent();
    
    // Declare the dummy function
    declareVarStoreFunction(M);
    
    // Find debug info for allocas
    findAllocaDebugInfo(F);
    
    // Transform stores
    transformStores(F);
    
    // Fix up no-argument calls
    fixupNoArgCalls(F);

    bool Modified = true;
    return Modified;
}

// Old Pass Manager Implementation (Clang 15 and 16)
#if CLANG_VER < 17
namespace {
    struct VarStorePass : public FunctionPass {
        static char ID;
        VarStorePassImpl impl;
        
        VarStorePass() : FunctionPass(ID) {}
        
        bool runOnFunction(Function &F) override {
            return impl.runOnFunction(F);
        }
    };
}

char VarStorePass::ID = 0;
static RegisterPass<VarStorePass> Y("var-store", "Transform stores to local pointer variables", false, false);
#endif

// New Pass Manager Implementation (Clang 17+)
#if CLANG_VER >= 17
namespace {
    struct VarStorePassNewPM : public PassInfoMixin<VarStorePassNewPM> {
        VarStorePassImpl impl;
        
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
            bool Modified = impl.runOnFunction(F);
            return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
        }
    };
}

// Register with new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "VarStorePass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "var-store") {
                        FPM.addPass(VarStorePassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
