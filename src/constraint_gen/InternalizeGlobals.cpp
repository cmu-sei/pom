//
// <legal></legal>
//


#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include <set>

// New pass manager includes (for Clang 17+)
#if CLANG_VER >= 17
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#endif

using namespace llvm;

// Command line option to specify globals to preserve
static cl::list<std::string> PreserveGlobals("internalize-global-preserve",
    cl::desc("Preserve these global variables (comma-separated)"),
    cl::CommaSeparated);

class InternalizeGlobalsImpl {
public:
    InternalizeGlobalsImpl() = default;

    bool runOnModule(Module &M) {
        bool Changed = false;

        // Build set of globals to preserve
        std::set<std::string> PreserveSet(PreserveGlobals.begin(), PreserveGlobals.end());

        // Iterate over all global variables
        for (GlobalVariable &GV : M.globals()) {
            // Skip if already internal or has local linkage
            if (GV.hasLocalLinkage())
                continue;

            // Skip if in preserve list
            if (PreserveSet.count(GV.getName().str()))
                continue;

            // Skip if it's a declaration (not defined in this module)
            if (GV.isDeclaration())
                continue;

            // Internalize the global variable
            GV.setLinkage(GlobalValue::InternalLinkage);
            Changed = true;
        }

        return Changed;
    }
};

// Old Pass Manager Implementation (Clang 15)
#if CLANG_VER < 17
namespace {
    struct InternalizeGlobalsPass : public ModulePass {
        static char ID;
        InternalizeGlobalsImpl impl;

        InternalizeGlobalsPass() : ModulePass(ID) {}

        bool runOnModule(Module &M) override {
            return impl.runOnModule(M);
        }
    };
}

char InternalizeGlobalsPass::ID = 0;
static RegisterPass<InternalizeGlobalsPass> X("internalize-globals",
    "Internalize global variables only", false, false);
#endif

// New Pass Manager Implementation (Clang 17+)
#if CLANG_VER >= 17
namespace {
    struct InternalizeGlobalsPassNewPM : public PassInfoMixin<InternalizeGlobalsPassNewPM> {
        InternalizeGlobalsImpl impl;

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            bool Changed = impl.runOnModule(M);
            return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
        }
    };
}

// Register with new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "InternalizeGlobalsPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "internalize-globals") {
                        MPM.addPass(InternalizeGlobalsPassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
