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
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <set>

// New pass manager includes (for Clang 17+)
#if CLANG_VER >= 17
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#endif

using namespace llvm;

class FuncPtrPassImpl {
public:
    // Maps each Value to the set of Functions it might point to
    DenseMap<Value*, std::set<Function*>> pointsToSets;

    // Tracks values that have unknown sources (e.g., loaded from memory)
    DenseSet<Value*> hasUnknownSource;

    FuncPtrPassImpl() = default;

    bool runOnModule(Module &M) {
        bool changed = false;

        // Initialize: function addresses point to themselves
        for (Function &F : M) {
            if (!F.isDeclaration()) {
                pointsToSets[&F].insert(&F);
            }
        }

        // Fixed-point iteration
        bool iterationChanged = true;
        while (iterationChanged) {
            iterationChanged = false;

            for (Function &F : M) {
                if (F.isDeclaration()) continue;

                for (BasicBlock &BB : F) {
                    for (Instruction &I : BB) {
                        if (propagateInstruction(&I)) {
                            iterationChanged = true;
                        }
                    }
                }
            }
        }

        // Add metadata to indirect calls
        for (Function &F : M) {
            if (F.isDeclaration()) continue;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                        if (annotateCall(CI, M.getContext())) {
                            changed = true;
                        }
                    }
                }
            }
        }

        return changed; // We modify IR (add metadata)
    }

private:
    // Check if a type is a function pointer
    bool isFunctionPointerType(Type *Ty) {
        if (auto *PtrTy = dyn_cast<PointerType>(Ty)) {
            return true; // In opaque pointer mode, we'll check values
        }
        return false;
    }

    // Propagate points-to information for a single instruction
    bool propagateInstruction(Instruction *I) {
        bool changed = false;

        if (auto *LI = dyn_cast<LoadInst>(I)) {
            // Load from memory: mark as having unknown source
            if (hasUnknownSource.insert(LI).second) {
                changed = true;
            }
        } else if (auto *PHI = dyn_cast<PHINode>(I)) {
            // PHI: union of all incoming values
            for (unsigned i = 0; i < PHI->getNumIncomingValues(); ++i) {
                Value *V = PHI->getIncomingValue(i);
                if (merge(PHI, V)) {
                    changed = true;
                }
            }
        } else if (auto *SI = dyn_cast<SelectInst>(I)) {
            // Select: union of true and false values
            if (merge(SI, SI->getTrueValue())) {
                changed = true;
            }
            if (merge(SI, SI->getFalseValue())) {
                changed = true;
            }
        } else if (auto *CI = dyn_cast<CallInst>(I)) {
            // Propagate actual arguments to formal parameters
            Function *DirectCallee = CI->getCalledFunction();
            if (DirectCallee) {
                if (DirectCallee->getName().str() == std::string("__pom_var_store")) {
                    Value *Actual = CI->getArgOperand(0);
                    if (merge(CI, Actual)) {
                        changed = true;
                    }
                } else {
                    changed |= propagateArguments(CI, DirectCallee);
                    changed |= propagateReturn(CI, DirectCallee);
                }
            } else {
                // Indirect call - propagate to all possible callees
                Value *CalledValue = CI->getCalledOperand();
                if (pointsToSets.count(CalledValue)) {
                    for (Function *F : pointsToSets[CalledValue]) {
                        changed |= propagateArguments(CI, F);
                        changed |= propagateReturn(CI, F);
                    }
                }
            }
        } else if (auto *BC = dyn_cast<BitCastInst>(I)) {
            // Bitcast: propagate through
            if (merge(BC, BC->getOperand(0))) {
                changed = true;
            }
        } else if (auto *II = dyn_cast<IntToPtrInst>(I)) {
            // IntToPtr: mark as having unknown source (could be anything)
            if (hasUnknownSource.insert(II).second) {
                changed = true;
            }
        } else if (auto *PI = dyn_cast<PtrToIntInst>(I)) {
            // PtrToInt: propagate through
            if (merge(PI, PI->getOperand(0))) {
                changed = true;
            }
        }

        return changed;
    }

    // Merge points-to sets: dest gets everything from src
    bool merge(Value *dest, Value *src) {
        bool changed = false;

        // Propagate known functions
        if (pointsToSets.count(src)) {
            for (Function *F : pointsToSets[src]) {
                if (pointsToSets[dest].insert(F).second) {
                    changed = true;
                }
            }
        }

        // Propagate unknown source flag
        if (hasUnknownSource.count(src)) {
            if (hasUnknownSource.insert(dest).second) {
                changed = true;
            }
        }

        return changed;
    }

    // Propagate return value from function to call site
    bool propagateReturn(CallInst *CI, Function *Callee) {
        bool changed = false;

        // Find all return instructions in the callee
        for (BasicBlock &BB : *Callee) {
            if (ReturnInst *RI = dyn_cast<ReturnInst>(BB.getTerminator())) {
                if (Value *RetVal = RI->getReturnValue()) {
                    if (merge(CI, RetVal)) {
                        changed = true;
                    }
                }
            }
        }

        return changed;
    }

    // Propagate actual arguments to formal parameters
    bool propagateArguments(CallInst *CI, Function *Callee) {
        bool changed = false;

        unsigned numArgs = CI->arg_size();
        unsigned numParams = Callee->arg_size();
        unsigned limit = std::min(numArgs, numParams);

        for (unsigned i = 0; i < limit; ++i) {
            Value *Actual = CI->getArgOperand(i);
            Argument *Formal = Callee->getArg(i);

            if (merge(Formal, Actual)) {
                changed = true;
            }
        }

        return changed;
    }

    Function* getCalledFunc(CallBase* CB) {
        return dyn_cast<Function>(CB->getCalledOperand()->stripPointerCasts());
    }

    // Annotate call instruction with possible callees metadata
    bool annotateCall(CallInst *CI, LLVMContext &Ctx) {
        // Only annotate indirect calls (calls through function pointers)
        if (getCalledFunc(CI)) {
            return false; // Direct call, skip
        }

        Value *CalledValue = CI->getCalledOperand();
        bool hasKnownTargets = pointsToSets.count(CalledValue) &&
                               !pointsToSets[CalledValue].empty();
        bool hasUnknown = hasUnknownSource.count(CalledValue);

        if (!hasKnownTargets && !hasUnknown) {
            return false; // No information available
        }

        // Create metadata with function names
        std::vector<Metadata*> FuncNames;

        // Add known function targets
        if (hasKnownTargets) {
            for (Function *F : pointsToSets[CalledValue]) {
                FuncNames.push_back(MDString::get(Ctx, F->getName()));
            }
        }

        // Add "???" if there are unknown sources
        if (hasUnknown) {
            FuncNames.push_back(MDString::get(Ctx, "???"));
        }

        MDNode *MD = MDNode::get(Ctx, FuncNames);
        CI->setMetadata("possible_callees", MD);

        return true;
    }
};

// Old Pass Manager Implementation (Clang 15)
#if CLANG_VER < 17
namespace {
    struct FuncPtrPassPass : public ModulePass {
        static char ID;
        FuncPtrPassImpl impl;

        FuncPtrPassPass() : ModulePass(ID) {}

        bool runOnModule(Module &M) override {
            return impl.runOnModule(M);
        }
    };
}

char FuncPtrPassPass::ID = 0;
static RegisterPass<FuncPtrPassPass> X("funcptr-analysis", "Function Pointer Analysis", false, false);
#endif

// New Pass Manager Implementation (Clang 17+)
#if CLANG_VER >= 17
namespace {
    struct FuncPtrPassPassNewPM : public PassInfoMixin<FuncPtrPassPassNewPM> {
        FuncPtrPassImpl impl;

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            impl.runOnModule(M);
            return PreservedAnalyses::none(); // We modify the IR (add metadata)
        }
    };
}

// Register with new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "FuncPtrPassPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "funcptr-analysis") {
                        MPM.addPass(FuncPtrPassPassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
