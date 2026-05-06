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


using namespace llvm;


class FreezeUBImpl {
public:
    bool runOnFunction(Function &F) {
        IRBuilder<> Builder(F.getContext());
        bool Modified = false;
        for (BasicBlock &BB : F) {
            for (Instruction &I : make_early_inc_range(BB)) {
                if (isa<FreezeInst>(&I)) {
                    continue;
                }
                
                PHINode *PHI = dyn_cast<PHINode>(&I);
                
                for (unsigned OpIdx = 0, NumOps = I.getNumOperands(); OpIdx != NumOps; ++OpIdx) {
                    Value *Op = I.getOperand(OpIdx);
                    if (isa<Constant>(Op) && dyn_cast<Constant>(Op)->isNullValue()) {
                        if (LoadInst* mem_inst = dyn_cast<LoadInst>(&I)) {
                            mem_inst->setVolatile(true);
                        } else if (StoreInst* mem_inst = dyn_cast<StoreInst>(&I)) {
                            mem_inst->setVolatile(true);
                        }
                    }
                    if (!isa<UndefValue>(Op) && !isa<PoisonValue>(Op)) {
                        continue;
                    }
                    
                    // Set insert point based on instruction type
                    if (PHI) {
                        // For PHI nodes, insert in the predecessor block before terminator
                        BasicBlock *PredBB = PHI->getIncomingBlock(OpIdx);
                        Builder.SetInsertPoint(PredBB->getTerminator());
                    } else {
                        // For normal instructions, insert before the instruction
                        Builder.SetInsertPoint(&I);
                    }
                    
                    Value *Frozen = Builder.CreateFreeze(Op, Op->getName() + ".frozen");
                    I.setOperand(OpIdx, Frozen);
                    Modified = true;
                }
            }
        }
        return Modified;
    }
};


// Old Pass Manager Implementation (Clang 15 and 16)
#if CLANG_VER < 17
namespace {
    struct FreezeUB : public FunctionPass {
        static char ID;
        FreezeUBImpl impl;
        
        FreezeUB() : FunctionPass(ID) {}
        
        bool runOnFunction(Function &F) override {
            return impl.runOnFunction(F);
        }
    };
}

char FreezeUB::ID = 0;
static RegisterPass<FreezeUB> Y("freeze-ub", "Freezes undef and poison values", false, false);
#endif

// New Pass Manager Implementation (Clang 17+)
#if CLANG_VER >= 17
namespace {
    struct FreezeUBNewPM : public PassInfoMixin<FreezeUBNewPM> {
        FreezeUBImpl impl;
        
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
        LLVM_PLUGIN_API_VERSION, "FreezeUB", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "freeze-ub") {
                        FPM.addPass(FreezeUBNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
