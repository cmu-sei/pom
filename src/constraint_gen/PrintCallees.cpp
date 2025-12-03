//
// <legal></legal>
//


#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/BinaryFormat/Dwarf.h"

#include <set>
#include <fstream>
#include <iostream>
#include <sstream>

// New pass manager includes (for Clang 17+)
#if CLANG_VER >= 17
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#endif

using namespace llvm;

bool has_prefix(const std::string& str, const std::string& prefix) {
    std::size_t len = prefix.length();
    return (str.size() >= len && str.compare(0, len, prefix) == 0);
}

// Command line option for output file
static cl::opt<std::string> OutputFile("output", cl::desc("Output file for call graph"), cl::value_desc("filename"));

class PrintCalleesImpl {
public:

    PrintCalleesImpl() = default;

    Function* getCalledFunc(CallBase* CB) {
        return dyn_cast<Function>(CB->getCalledOperand()->stripPointerCasts());
    }

    bool runOnModule(Module &M) {
        std::ofstream fileStream;
        std::ostream* outStream = nullptr;

        // Initialize output stream
        if (OutputFile.empty()) {
            outStream = &std::cout;
        } else {
            fileStream.open(OutputFile, std::ios::out);
            if (!fileStream.is_open()) {
                errs() << "Error: Cannot open output file " << OutputFile << "\n";
                return false;
            }
            outStream = &fileStream;
        }

        std::set<Function*> seen_funcs;

        // Find all functions *defined* in the module
        for (Function &F : M) {
            // Skip function declarations (only process definitions)
            if (F.isDeclaration()) {
                continue;
            }
            seen_funcs.insert(&F);
        }

        // Process all functions in the module
        for (Function &F : M) {
            // Skip function declarations (only process definitions)
            if (F.isDeclaration()) {
                continue;
            }

            std::set<std::string> callees;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (auto *CB = dyn_cast<CallBase>(&I)) {
                        Function *calledFunc = getCalledFunc(CB);
                        if (!calledFunc) {
                            continue;
                        }
                        if (!seen_funcs.count(calledFunc)) {
                            continue;
                        }
                        callees.insert(calledFunc->getName().str());
                    }
                }
            }

            // Print the function name and its callees in the required format
            *outStream << F.getName().str() << ":";
            for (const auto &callee : callees) {
                if (has_prefix(callee, "llvm.")) {
                    continue;
                }
                *outStream << " " << callee;
            }
            *outStream << "\n";
        }

        return false; // We don't modify the IR
    }
};

// Old Pass Manager Implementation (Clang 15)
#if CLANG_VER < 17
namespace {
    struct PrintCalleesPass : public ModulePass {
        static char ID;
        PrintCalleesImpl impl;

        PrintCalleesPass() : ModulePass(ID) {}

        bool runOnModule(Module &M) override {
            return impl.runOnModule(M);
        }
    };
}

char PrintCalleesPass::ID = 0;
static RegisterPass<PrintCalleesPass> X("print-callees", "Print call graph", false, false);
#endif

// New Pass Manager Implementation (Clang 18+)
#if CLANG_VER >= 17
namespace {
    struct PrintCalleesPassNewPM : public PassInfoMixin<PrintCalleesPassNewPM> {
        PrintCalleesImpl impl;

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            impl.runOnModule(M);
            return PreservedAnalyses::all();
        }
    };
}

// Register with new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "PrintCalleesPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "print-callees") {
                        MPM.addPass(PrintCalleesPassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
