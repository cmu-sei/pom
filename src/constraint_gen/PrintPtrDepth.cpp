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
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/BinaryFormat/Dwarf.h"

#include <map>
#include <fstream>
#include <iostream>
#include <string>

// New pass manager includes (for Clang 17+)
#if CLANG_VER >= 17
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#endif

using namespace llvm;

// Command line option for output file
static cl::opt<std::string> OutputFile("output", cl::desc("Output file for pointer depth analysis"), cl::value_desc("filename"));

class PointerDepthImpl {
public:

    PointerDepthImpl() = default;

    // Skip qualifiers like const, volatile, typedef, etc.
    static const DIType* skipQualifiers(const DIType *Ty) {
        while (const auto *DT = dyn_cast<DIDerivedType>(Ty)) {
            switch (DT->getTag()) {
                case dwarf::DW_TAG_const_type:
                case dwarf::DW_TAG_volatile_type:
                case dwarf::DW_TAG_restrict_type:
                case dwarf::DW_TAG_typedef:
                case dwarf::DW_TAG_atomic_type:
                    Ty = DT->getBaseType();
                    continue;
                default:
                    return Ty;
            }
        }
        return Ty;
    }

    // Count pointer indirection levels from debug info
    static int countPointerIndirectionFromDI(const DIType *Ty) {
        int n = 0;
        while (Ty) {
            Ty = skipQualifiers(Ty);
            if (const auto *DT = dyn_cast<DIDerivedType>(Ty)) {
                if (DT->getTag() == dwarf::DW_TAG_pointer_type) {
                    ++n;
                    Ty = DT->getBaseType(); // keep peeling
                    continue;
                }
            }
            break;
        }
        return n;
    }

    // Get pointer depth for return type
    int getReturnTypePtrDepth(Function *F) {
        DISubprogram *SP = F->getSubprogram();
        if (!SP) return -1;

        DISubroutineType *ST = SP->getType();
        if (!ST) return -1;

        DITypeRefArray Types = ST->getTypeArray();
        if (Types.size() == 0) return -1;

        DIType *ReturnType = Types[0];
        if (!ReturnType) return -1;

        return countPointerIndirectionFromDI(ReturnType);
    }

    // Get pointer depth for a specific argument by index
    int getArgTypePtrDepth(Function *F, unsigned argIdx) {
        DISubprogram *SP = F->getSubprogram();
        if (!SP) return -1;

        DISubroutineType *ST = SP->getType();
        if (!ST) return -1;

        DITypeRefArray Types = ST->getTypeArray();
        if (Types.size() == 0) return -1;

        // In DISubroutineType, index 0 = return type; params start at 1
        size_t diIdx = static_cast<size_t>(argIdx + 1);
        if (diIdx >= Types.size()) return -1;

        DIType *ArgTy = Types[diIdx];
        if (!ArgTy) return -1;

        return countPointerIndirectionFromDI(ArgTy);
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

        // Store results for all functions
        std::map<std::string, std::map<std::string, int>> results;

        // Process all functions defined in the module
        for (Function &F : M) {
            // Skip function declarations (only process definitions)
            if (F.isDeclaration()) {
                continue;
            }

            std::string funcName = F.getName().str();
            std::map<std::string, int> depths;

            // Get return type pointer depth
            int retDepth;
            if (F.getReturnType()->isVoidTy()) {
                retDepth = 0;
            } else {
                retDepth = getReturnTypePtrDepth(&F);
            }
            if (retDepth >= 0) {
                depths["return"] = retDepth;
            } else {
                errs() << "Error getting ptr depth for return of func '" << funcName << "'\n";
            }

            // Get pointer depth for each argument
            unsigned argIdx = 0;
            for (Argument &Arg : F.args()) {
                std::string argName;
                if (Arg.hasName()) {
                    argName = Arg.getName().str();
                } else {
                    argName = "arg" + std::to_string(argIdx);
                }

                int argDepth = getArgTypePtrDepth(&F, argIdx);
                if (argDepth >= 0) {
                    depths[argName] = argDepth;
                } else {
                    errs() << "Error getting ptr depth for arg '" << argName << "' of func '" << funcName << "'\n";
                }
                argIdx++;
            }

            results[funcName] = depths;
        }

        // Output in JSON format
        *outStream << "{\n";
        bool firstFunc = true;
        for (const auto &funcEntry : results) {
            if (!firstFunc) {
                *outStream << ",\n";
            }
            firstFunc = false;

            *outStream << "  \"" << funcEntry.first << "\": {\n";

            bool firstArg = true;
            for (const auto &argEntry : funcEntry.second) {
                if (!firstArg) {
                    *outStream << ",\n";
                }
                firstArg = false;

                *outStream << "    \"" << argEntry.first << "\": " << argEntry.second;
            }
            *outStream << "\n  }";
        }
        *outStream << "\n}\n";

        return false; // We don't modify the IR
    }
};

// Old Pass Manager Implementation (Clang 15)
#if CLANG_VER < 17
namespace {
    struct PointerDepthPass : public ModulePass {
        static char ID;
        PointerDepthImpl impl;

        PointerDepthPass() : ModulePass(ID) {}

        bool runOnModule(Module &M) override {
            return impl.runOnModule(M);
        }
    };
}

char PointerDepthPass::ID = 0;
static RegisterPass<PointerDepthPass> X("pointer-depth", "Analyze pointer depths of function arguments and return values", false, false);
#endif

// New Pass Manager Implementation (Clang 17+)
#if CLANG_VER >= 17
namespace {
    struct PointerDepthPassNewPM : public PassInfoMixin<PointerDepthPassNewPM> {
        PointerDepthImpl impl;

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
        LLVM_PLUGIN_API_VERSION, "PointerDepthPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "pointer-depth") {
                        MPM.addPass(PointerDepthPassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
