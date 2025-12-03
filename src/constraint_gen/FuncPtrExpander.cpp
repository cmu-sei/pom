//
// <legal></legal>
//


#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

// New pass manager includes (for Clang 17+)
#if CLANG_VER >= 17
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#endif

using namespace llvm;

class FuncPtrExpanderImpl {
public:
    FuncPtrExpanderImpl() = default;

    bool runOnModule(Module &M) {
        bool changed = false;

        // Collect all indirect calls to expand (can't modify while iterating)
        std::vector<CallInst*> callsToExpand;

        for (Function &F : M) {
            if (F.isDeclaration()) continue;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                        // Check if this is an indirect call with metadata
                        if (!CI->getCalledFunction() &&
                            CI->getMetadata("possible_callees")) {
                            callsToExpand.push_back(CI);
                        }
                    }
                }
            }
        }

        // Expand each indirect call
        for (CallInst *CI : callsToExpand) {
            if (expandIndirectCall(CI, M)) {
                changed = true;
            }
        }

        return changed;
    }

private:
    bool expandIndirectCall(CallInst *CI, Module &M) {
        MDNode *MD = CI->getMetadata("possible_callees");
        if (!MD || MD->getNumOperands() == 0) {
            return false;
        }

        // Get debug location from original call
        DebugLoc DL = CI->getDebugLoc();

        // Parse metadata to get list of possible callees
        std::vector<Function*> possibleCallees;
        bool hasUnknown = false;

        for (unsigned i = 0; i < MD->getNumOperands(); ++i) {
            MDString *FuncNameMD = dyn_cast<MDString>(MD->getOperand(i));
            if (!FuncNameMD) continue;

            StringRef FuncName = FuncNameMD->getString();
            if (FuncName == "???") {
                hasUnknown = true;
            } else {
                Function *F = M.getFunction(FuncName);
                if (F) {
                    possibleCallees.push_back(F);
                }
            }
        }

        // If no known callees, don't transform
        if (possibleCallees.empty()) {
            return false;
        }

        // Get the called value (function pointer)
        Value *CalledValue = CI->getCalledOperand();

        // Get the parent basic block and function
        BasicBlock *OrigBB = CI->getParent();
        Function *ParentFunc = OrigBB->getParent();

        // Split the basic block after the call instruction
        BasicBlock *MergeBB = OrigBB->splitBasicBlock(CI->getNextNode(), "merge");

        // Remove the unconditional branch that splitBasicBlock added
        OrigBB->getTerminator()->eraseFromParent();

        IRBuilder<> Builder(OrigBB);

        // Create basic blocks for each case
        std::vector<BasicBlock*> callBlocks;
        std::vector<BasicBlock*> checkBlocks;

        for (size_t i = 0; i < possibleCallees.size(); ++i) {
            checkBlocks.push_back(BasicBlock::Create(
                M.getContext(),
                "check." + possibleCallees[i]->getName(),
                ParentFunc,
                MergeBB
            ));
            callBlocks.push_back(BasicBlock::Create(
                M.getContext(),
                "call." + possibleCallees[i]->getName(),
                ParentFunc,
                MergeBB
            ));
        }

        // Create else block
        BasicBlock *ElseBB = BasicBlock::Create(
            M.getContext(),
            "else",
            ParentFunc,
            MergeBB
        );

        // Build the if-else chain
        Builder.SetInsertPoint(OrigBB);
        Instruction *Branch = Builder.CreateBr(checkBlocks[0]);
        Branch->setDebugLoc(DL);

        // Values for PHI node (collect all results)
        std::vector<Value*> incomingValues;
        std::vector<BasicBlock*> incomingBlocks;

        for (size_t i = 0; i < possibleCallees.size(); ++i) {
            // Build check block
            Builder.SetInsertPoint(checkBlocks[i]);
            Value *Cmp = Builder.CreateICmpEQ(CalledValue, possibleCallees[i]);
            if (Instruction *CmpInst = dyn_cast<Instruction>(Cmp)) {
                CmpInst->setDebugLoc(DL);
            }

            BasicBlock *NextCheck = (i + 1 < possibleCallees.size())
                                    ? checkBlocks[i + 1]
                                    : ElseBB;
            Instruction *CondBr = Builder.CreateCondBr(Cmp, callBlocks[i], NextCheck);
            CondBr->setDebugLoc(DL);

            // Build call block
            Builder.SetInsertPoint(callBlocks[i]);

            // Create direct call with same arguments
            std::vector<Value*> Args;
            for (unsigned j = 0; j < CI->arg_size(); ++j) {
                Args.push_back(CI->getArgOperand(j));
            }

            CallInst *DirectCall = Builder.CreateCall(possibleCallees[i], Args);
            DirectCall->setCallingConv(CI->getCallingConv());
            DirectCall->setAttributes(CI->getAttributes());
            DirectCall->setTailCallKind(CI->getTailCallKind());
            DirectCall->setDebugLoc(DL);  // Set debug location

            Instruction *CallBr = Builder.CreateBr(MergeBB);
            CallBr->setDebugLoc(DL);

            if (!CI->getType()->isVoidTy()) {
                incomingValues.push_back(DirectCall);
                incomingBlocks.push_back(callBlocks[i]);
            }
        }

        // Build else block
        Builder.SetInsertPoint(ElseBB);

        if (hasUnknown) {
            // Keep the indirect call
            std::vector<Value*> Args;
            for (unsigned j = 0; j < CI->arg_size(); ++j) {
                Args.push_back(CI->getArgOperand(j));
            }

            CallInst *IndirectCall = Builder.CreateCall(
                CI->getFunctionType(),
                CalledValue,
                Args
            );
            IndirectCall->setCallingConv(CI->getCallingConv());
            IndirectCall->setAttributes(CI->getAttributes());
            IndirectCall->setTailCallKind(CI->getTailCallKind());
            IndirectCall->setDebugLoc(DL);  // Set debug location

            Instruction *ElseBr = Builder.CreateBr(MergeBB);
            ElseBr->setDebugLoc(DL);

            if (!CI->getType()->isVoidTy()) {
                incomingValues.push_back(IndirectCall);
                incomingBlocks.push_back(ElseBB);
            }
        } else {
            // Call abort()
            FunctionType *AbortTy = FunctionType::get(
                Type::getVoidTy(M.getContext()),
                false
            );
            FunctionCallee AbortFunc = M.getOrInsertFunction("abort", AbortTy);
            CallInst *AbortCall = Builder.CreateCall(AbortFunc);
            AbortCall->setDebugLoc(DL);  // Set debug location

            Instruction *Unreach = Builder.CreateUnreachable();
            Unreach->setDebugLoc(DL);

            // Note: no branch to merge, and no value for PHI
        }

        // Create PHI node in merge block if the call has a return value
        if (!CI->getType()->isVoidTy() && !incomingValues.empty()) {
            Builder.SetInsertPoint(&MergeBB->front());
            PHINode *PHI = Builder.CreatePHI(CI->getType(), incomingValues.size());
            PHI->setDebugLoc(DL);  // Set debug location
            auto& Ctx = M.getContext();
            PHI->setMetadata("phi_origin", MDNode::get(Ctx, MDString::get(Ctx, "indirect_call_expansion")));

            for (size_t i = 0; i < incomingValues.size(); ++i) {
                PHI->addIncoming(incomingValues[i], incomingBlocks[i]);
            }

            // Replace all uses of the original call with the PHI
            CI->replaceAllUsesWith(PHI);
        }

        // Remove the original call instruction
        CI->eraseFromParent();

        return true;
    }
};

// Old Pass Manager Implementation (Clang 15)
#if CLANG_VER < 17
namespace {
    struct FuncPtrExpanderPass : public ModulePass {
        static char ID;
        FuncPtrExpanderImpl impl;

        FuncPtrExpanderPass() : ModulePass(ID) {}

        bool runOnModule(Module &M) override {
            return impl.runOnModule(M);
        }
    };
}

char FuncPtrExpanderPass::ID = 0;
static RegisterPass<FuncPtrExpanderPass> X("expand-indirect-calls",
    "Expand Indirect Function Calls", false, false);
#endif

// New Pass Manager Implementation (Clang 17+)
#if CLANG_VER >= 17
namespace {
    struct FuncPtrExpanderPassNewPM : public PassInfoMixin<FuncPtrExpanderPassNewPM> {
        FuncPtrExpanderImpl impl;

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            impl.runOnModule(M);
            return PreservedAnalyses::none(); // We modify the IR significantly
        }
    };
}

// Register with new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "FuncPtrExpanderPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "expand-indirect-calls") {
                        MPM.addPass(FuncPtrExpanderPassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
