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

// New pass manager includes (for Clang 18+)
#if CLANG_VER >= 17
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#endif

#include <fstream>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include <utility>
#include <algorithm>
#include <regex>


#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DataLayout.h"

#include "bitvector.h"


using namespace llvm;


std::string replace_substr(std::string big_str, const std::string& old_ss, const std::string& new_ss) {
    size_t pos = big_str.find(old_ss);
    if (pos != std::string::npos) {
        big_str.replace(pos, old_ss.length(), new_ss);
    }
    return big_str;
}

void rtrim_inplace(std::string &s) {
    // Find the first non-whitespace character from the end
    auto begin = std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) {
            return !std::isspace(ch);
        }
    ).base();
    s.erase(begin, s.end());
}

std::string basename(const std::string& path) {
    size_t lastSlash = path.find_last_of("/");
    if (std::string::npos != lastSlash) {
        return path.substr(lastSlash + 1);
    }
    return path;
}

//////////////////////////////////////////////////////////////////////////////
// { Code for getting field names of structs
//////////////////////////////////////////////////////////////////////////////

namespace {

static std::string stripStructTagPrefix(StringRef Name) {
  // LLVM IR names for C structs are typically like "struct.foo"
  if (Name.startswith("struct."))
    return Name.drop_front(strlen("struct.")).str();
  if (Name.startswith("%struct."))
    return Name.drop_front(strlen("%struct.")).str();
  return Name.str();
}

bool has_prefix(const std::string& str, const std::string& prefix) {
    std::size_t len = prefix.length();
    return (str.size() >= len && str.compare(0, len, prefix) == 0);
}
bool has_suffix(const std::string& str, const std::string& suffix) {
    std::size_t len = suffix.length();
    return (str.size() >= len && str.compare(str.size() - len, len, suffix) == 0);
}
std::string strip_suffix(const std::string& s, size_t suffix_len) {
    return s.substr(0, s.length() - suffix_len);
}

struct GEPStructIndex {
  StructType *ST = nullptr;
  unsigned FieldIndex = 0;
};

// Finds the (StructType, field index) pair for the struct step within this GEP.
// Works with opaque pointers because we use gep_type_iterator rather than pointee types.
static std::optional<GEPStructIndex> findStructAndIndexInGEP(const GetElementPtrInst *GEP) {
  for (gep_type_iterator GTI = gep_type_begin(GEP), E = gep_type_end(GEP); GTI != E; ++GTI) {
    if (StructType *CurST = GTI.getStructTypeOrNull()) {
      const Value *IdxV = GTI.getOperand();
      const auto *CI = dyn_cast<ConstantInt>(IdxV);
      if (!CI) return std::nullopt; // non-constant field index (rare at -O0, but possible)
      return GEPStructIndex{CurST, static_cast<unsigned>(CI->getZExtValue())};
    }
  }
  return std::nullopt;
}

static uint64_t getFieldOffsetBits(StructType *ST, unsigned FieldIndex, const DataLayout &DL) {
  const StructLayout *SL = DL.getStructLayout(ST);
  return static_cast<uint64_t>(SL->getElementOffset(FieldIndex)) * 8ULL;
}

// Find corresponding DICompositeType for an LLVM StructType
static const std::vector<DICompositeType*>& find_DI_for_struct(StructType *ST, Module &M, const DataLayout &DL) {
  static const std::vector<DICompositeType*> empty_vector; // For early returns
  const uint64_t STSizeBits = DL.getTypeAllocSizeInBits(ST);
  const unsigned NumElems   = ST->getNumElements();
  if (!ST->hasName()) {
    return empty_vector;
  }
  const std::string IRName  = stripStructTagPrefix(ST->getName());

  static std::map<std::string, std::vector<DICompositeType*>> cache;
  
  // Check cache first - return reference to cached entry
  auto it = cache.find(IRName);
  if (it != cache.end()) {
    return it->second;
  }

  DebugInfoFinder DIF;
  DIF.processModule(M);

  DICompositeType *Best = nullptr;
  int BestScore = -1;

  std::vector<DICompositeType*> ret;

  for (DIType *T : DIF.types()) {
    auto *CT = dyn_cast<DICompositeType>(T);
    if (!CT) continue;
    if (CT->getTag() != dwarf::DW_TAG_structure_type) continue;

    // Collect candidate attributes
    const std::string DIName = CT->getName().str();
    const uint64_t DISzBits  = CT->getSizeInBits();
    DINodeArray Elems        = CT->getElements();
    unsigned DIMemberCount   = 0;
    for (auto *N : Elems) {
      if (isa<DIDerivedType>(N) && cast<DIDerivedType>(N)->getTag() == dwarf::DW_TAG_member) {
        ++DIMemberCount;
      }
    }

    bool has_mismatched = false;
    if (!IRName.empty() && !DIName.empty() && IRName == DIName) {
        if (DISzBits != STSizeBits) {
            has_mismatched = true;
            continue;
        }
        if (DIMemberCount != NumElems) {
            has_mismatched = true;
            continue;
        }
        ret.push_back(CT);
    }
    if (ret.size() == 0 && has_mismatched) {
        errs() << "Warning: No matching struct DI info for struct '" << IRName << "' was found, "
               << "but mismatched (in element count and/or size in bits) DI info was found.\n";
    }
  }

  cache[IRName] = std::move(ret);  // Move to avoid copy
  return cache[IRName];
}

static std::string pickFieldNameByOffset(DICompositeType *CT, uint64_t OffsetBits) {
  if (!CT) return {};
  for (DINode *N : CT->getElements()) {
    auto *M = dyn_cast<DIDerivedType>(N);
    if (!M) continue;
    if (M->getTag() != dwarf::DW_TAG_member) continue;
    if (M->getOffsetInBits() == OffsetBits) {
      return M->getName().str();
    }
  }
  return {};
}

} // namespace

// === Public API ===
// Returns (struct_name, field_name), using DWARF debug info when available.
// On failure, returns best-effort fallbacks (e.g., IR struct name and "field#N") or empty strings.
std::pair<std::string, std::string>
get_GEP_field_info(const GetElementPtrInst *GEP) {
  static std::set<std::string> already_warned;
  if (!GEP) return {"", ""};

  const Function *F = GEP->getFunction();
  if (!F) return {"", ""};
  Module *M = const_cast<Module *>(F->getParent());
  if (!M) return {"", ""};

  const DataLayout &DL = M->getDataLayout();

  // 1) Identify the struct and the field index being accessed by this GEP.
  auto SI = findStructAndIndexInGEP(GEP);
  if (!SI.has_value() || !SI->ST) {
    return {"", ""};
  }

  StructType *ST = SI->ST;
  if (!ST->hasName()) {
    return {"", ""};
  }
  const std::string StructName = stripStructTagPrefix(ST->getName());
  const unsigned FieldIndex = SI->FieldIndex;
  if (FieldIndex >= ST->getNumElements()) {
    // Out-of-range; nothing we can do.
    return {StructName, ""};
  }

  // 2) Compute the byte/bit offset of that field.
  const uint64_t FieldOffBits = getFieldOffsetBits(ST, FieldIndex, DL);

  // 3) Find matching DICompositeType and the member at that offset.
  const std::vector<DICompositeType*>& CT_cands = find_DI_for_struct(ST, *M, DL);
  std::set<std::string> cand_field_names;
  for (DICompositeType* CT : CT_cands) {
    std::string CandFieldName = pickFieldNameByOffset(CT, FieldOffBits);
    cand_field_names.insert(CandFieldName);
  }
  if (cand_field_names.size() != 1) {
    if (already_warned.count(StructName) == 0) {
        already_warned.insert(StructName);
        if (cand_field_names.size() == 0) {
            errs() << "Warning: No struct info found for struct '" << StructName << "'\n";
        } else {
            errs() << "Warning: Conflicting struct info found for struct '" << StructName << "'\n";
        }
    }
  }
  std::string FieldName;
  if (cand_field_names.size() > 0) {
    FieldName = *cand_field_names.begin();
  }

  return {StructName, FieldName};
}
//////////////////////////////////////////////////////////////////////////////
// } End of code for getting field names of structs
//////////////////////////////////////////////////////////////////////////////






// Command line option for output file
static cl::opt<std::string> OutputFile("output", cl::desc("Output file for constraints"), cl::value_desc("filename"));
static cl::opt<std::string> NumberedIR("numir",  cl::desc("Numbered IR instructions"),    cl::value_desc("filename"));
static cl::opt<bool> DisableNullCheck(
    "no-check-null",
    cl::desc("Disable check for null dereferences"),
    cl::init(false));
static cl::opt<bool> DisableNullCheck2(
    "no-null-check",
    cl::desc("Synonym of '-no-check-null'"),
    cl::init(false));
static cl::opt<bool> DisableMemLeak(
    "no-mem-leak",
    cl::desc("Disable check for memory leaks"),
    cl::init(false));
static cl::opt<bool> DisableBorrowChecks(
    "no-borrow-check",
    cl::desc("Disable checks for borrows"),
    cl::init(false));
static cl::opt<bool> DisablePomLocals(
    "no-pom-yaml-locals",
    cl::desc("Disable checking of POM YAML specifications for local vars"),
    cl::init(false));
static cl::opt<bool> DisablePomStructs(
    "no-pom-yaml-structs",
    cl::desc("Disable checking of POM YAML specifications for struct fields"),
    cl::init(false));
static cl::opt<bool> DisableLiveness(
    "no-liveness",
    cl::desc("Disable liveness analysis"),
    cl::init(false));
static cl::opt<bool> NoWarnPhis(
    "no-warn-phis",
    cl::desc("Disable warnings about missing metadata for phi instructions"),
    cl::init(false));
static cl::opt<bool> NoWarnArgNames(
    "no-warn-arg-names",
    cl::desc("Disable warnings about missing names for function arguments"),
    cl::init(false));
static cl::opt<bool> NoWarnFuncPtr (
    "no-warn-func-ptr",
    cl::desc("Disable warnings about function pointers"),
    cl::init(false));
static cl::opt<size_t> TargetDebugPoint(
    "debug-point",
    cl::desc("Prints debug info at specified point"),
    cl::init(false));
static cl::opt<size_t> TargetDebugInstId(
    "debug-inst",
    cl::desc("Prints debug info at specified instruction"),
    cl::init(false));
static cl::opt<bool> DebugLiveness(
    "debug-liveness",
    cl::desc("Prints debug info for liveness analysis"),
    cl::init(false));
static cl::opt<bool> WholeProg(
    "whole-prog",
    cl::desc("Whole-program analysis, iterating until a fixed point is reached."),
    cl::init(false));
static cl::opt<bool> GloUniq(
    "glo-uniq",
    cl::desc("Use globally unique variable names (as opposed to per-function unique); this is now always on."),
    cl::init(true));
static cl::opt<bool> CheckPomComplete(
    "check-pom-complete",
    cl::desc("Check that the '.pom.yml' file is complete"),
    cl::init(false));
static cl::opt<std::string> ArgNameFile(
    "arg-name-file",
    cl::desc("File with names of arguments of std lib funcs"),
    cl::value_desc("filename"));
static cl::opt<std::string> PomPropsFile(
    "pom-props",
    cl::desc("POM '.props' file"),
    cl::value_desc("filename"));


template <typename T>
class Worklist {
public:
    std::queue<T> q;
    std::set<T> s;

    // Add item only if it doesn't already exist
    void push(const T& item) {
        if (s.find(item) == s.end()) {
            q.push(item);
            s.insert(item);
        }
    }

    // Remove and return the front item
    T pop() {
        T item = q.front();
        q.pop();
        s.erase(item);
        return item;
    }

    // Check if worklist is empty
    bool empty() const {
        return q.empty();
    }
};

// The first part of a cpath is a variable name or "$func_name::return" or "$func_name::args::$arg_name".
// Each additional part of a cpath is either:
//  - "[0]" (for pointer deref or array indexing) or
//  - ".fieldname" for struct fields.

typedef uint32_t CPathPart;
#define CPathPartMax UINT32_MAX
#define DEREF_PART_ID 1
typedef std::vector<CPathPart> CPath;

struct Borrow {
    CPath to;
    CPath from;
    
    bool operator==(const Borrow& other) const {
        return to == other.to && from == other.from;
    }

    bool operator!=(const Borrow& other) const {
        return !(*this == other);
    }
};

struct BorrowLessThanBySrcThenDest {
    bool operator()(const Borrow& lhs, const Borrow& rhs) const {
        if (lhs.from < rhs.from) {return true;}
        if (rhs.from < lhs.from) {return false;}
        if (lhs.to < rhs.to) {return true;}
        return false;
    }
};

struct BorrowLessThanByDestThenSrc {
    bool operator()(const Borrow& lhs, const Borrow& rhs) const {
        if (lhs.to < rhs.to) {return true;}
        if (rhs.to < lhs.to) {return false;}
        if (lhs.from < rhs.from) {return true;}
        return false;
    }
};

inline bool starts_with(const CPath& mainCPath, const CPath& prefix) {
    // Returns true if mainCPath starts with prefix, false otherwise.
    return (mainCPath.size() >= prefix.size() && 
            std::equal(prefix.begin(), prefix.end(), mainCPath.begin()));
}

struct BorrowInfo {
    std::set<Borrow, BorrowLessThanBySrcThenDest> borrows_by_src;
    std::set<Borrow, BorrowLessThanByDestThenSrc> borrows_by_dest;
    std::map<CPathPart /*base_var*/, bool> is_dyn_exclusive; // Whether only a single borrow exists dynamically for base_var.
    std::map<Borrow, bool, BorrowLessThanBySrcThenDest> is_must_borrow; // Whether this borrow is known to definitely be in effect.
    std::set<CPath> waiting_to_die;

    void add(Borrow borrow) {
        borrows_by_src.insert(borrow);
        borrows_by_dest.insert(borrow);
    }

    bool get_is_dyn_exclusive(CPathPart base_var, bool default_val) const {
        auto it = is_dyn_exclusive.find(base_var);
        return (it != is_dyn_exclusive.end()) ? it->second : default_val;
    }
    
    bool get_is_must_borrow(const Borrow& b) const {
        auto it = is_must_borrow.find(b);
        return (it != is_must_borrow.end()) ? it->second : false;
    };

    bool operator==(const BorrowInfo& other) const {
        // Only need to check one borrow set since they contain the same elements
        if (borrows_by_src != other.borrows_by_src) return false;
        if (is_dyn_exclusive != other.is_dyn_exclusive) return false;
        if (is_must_borrow != other.is_must_borrow) return false;
        return true;
    }

    bool operator!=(const BorrowInfo& other) const {
        return !(*this == other);
    }
};


inline bool do_cpaths_conflict(const CPath& a, const CPath& b) {
    return starts_with(a, b) || starts_with(b, a);
}

// Breadth-first search from start, following edges whose `from` aliases the current node,
// and stepping to their `to`. Returns true if we can reach any node that aliases `target`.
static bool has_path_alias_aware(const CPath& start,
                                 const CPath& target,
                                 const BorrowInfo& curBI)
{
    // Quick self-alias short-circuit: if start already aliases target,
    // then adding edge target<-start would immediately close a cycle.
    if (do_cpaths_conflict(start, target)) return true;

    // Visited set as a simple vector (graphs are typically small here).
    std::vector<CPath> visited;
    auto already_visited = [&](const CPath& n) -> bool {
        for (const auto& v : visited) {
            if (v == n) return true; // exact equality to control blow-up
        }
        return false;
    };

    std::queue<CPath> q;
    q.push(start);
    visited.push_back(start);

    while (!q.empty()) {
        CPath cur = q.front();
        q.pop();

        // Follow all edges whose `from` aliases `cur`
        for (const Borrow& b : curBI.borrows_by_src) {
            if (!do_cpaths_conflict(b.from, cur)) continue;

            // If we can reach a node aliasing target, there's a path back.
            if (do_cpaths_conflict(b.to, target)) return true;

            if (!already_visited(b.to)) {
                visited.push_back(b.to);
                q.push(b.to);
            }
        }
    }
    return false;
}

// Returns true if adding `borrow` to `curBI` would create a cycle in the borrow graph.
inline bool does_borrow_add_cycle(const Borrow& borrow, const BorrowInfo& curBI) {
    // We are about to add an edge: borrow.from -> borrow.to.
    // Check if there is an existing path from borrow.to back to borrow.from.
    return has_path_alias_aware(borrow.to, borrow.from, curBI);
}


#define MAX_PTR_DEPTH 6
#define MAX_CPATH_DEPTH 8

// Shared implementation class that contains all the core logic
class ConstraintGenImpl {
public:
    std::ofstream outFile;
    std::ofstream numirFile;
    std::set<std::string> seenCPaths; // Track C-paths that appear in constraints
    std::set<GlobalValue*> seen_globals;
    std::map<std::string, Value*> cpathToValue; // Inverse of getCPath
    std::map<Instruction*, unsigned> instToId; // Map instructions to unique IDs
    std::map<Value*, std::string> valueToSourceName; // Map LLVM values to source variable names
    std::map<Value*, unsigned> valueToVarId; // Map LLVM values to unique variable IDs
    std::set<std::string> argNameSet;
    std::set<std::string> argsToRealloc;
    std::set<Function*> already_renamed_func_values;
    bool already_renamed_cur_func_values;

    std::map<Instruction*, std::vector<Value*>> lastUseIn;  // lastUseIn[inst] is list of values that are not used again after inst.
    std::map<BasicBlock*, std::set<Value*>> liveAtStartSet; // liveAtStart[bb] indicates the values that are live at the start of bb.
    std::map<BasicBlock*, std::set<Value*>> liveAtEndSet;   // liveAtEnd[bb] indicates the values that are live at the end of bb.
    std::set<Value*> currentLiveVars;
    Function* curFunc;
    Instruction* curInstruction;
    unsigned curInstructionID;
    
    std::vector<std::string> pomPropLines;
    std::map<std::string, std::string> pomDepMutRet;
    std::unordered_set<std::string> pomFuncs;
    std::unordered_set<std::string> pomCPaths;
    std::map<std::string, std::map<std::string, std::set<std::string>>> pomBorrows;
    std::vector<std::string> pomMissingErrors;
    // pomBorrows[func_name][dest] = {sources from which dest borrows}
    bool hasUpdatedBorrows; // Whether pomBorrows has been updated
    
    std::set<std::string> seen_unsupported; // Unsupported features that we've seen.

    std::map<std::string, std::vector<std::string>> funcArgName;

    std::map<CallInst*, Value*> firstUseOfCall;

    std::unordered_map<std::string, CPathPart> cpath_part_str_to_id;
    std::unordered_map<CPathPart, std::string> cpath_part_id_to_str;
    std::unordered_map<std::string, CPath> cpath_parse_cache;

    std::unordered_map<BasicBlock*, BorrowInfo> borrows_at_end_of_bb;
    BorrowInfo curBI;

    bool alreadyInit = false;
    unsigned nextInstId = 1;
    unsigned nextVarId = 1;
    bool isDryRun;
    size_t debugPoint = 1;
    
    ConstraintGenImpl() = default;


    void reset_at_end_of_func() {
        seenCPaths.clear();
        seen_globals.clear();
        cpathToValue.clear();
        //instToId.clear();
        valueToSourceName.clear();
        //valueToVarId.clear();
        argNameSet.clear();
        lastUseIn.clear();
        liveAtStartSet.clear();
        liveAtEndSet.clear();
        currentLiveVars.clear();
        curFunc = nullptr;
        firstUseOfCall.clear();
        already_renamed_cur_func_values = false;
    }

    struct PtrCopyArgs {
        std::string dest;
        std::string src;
        std::string stmtPre;
        std::string stmtPost;
        std::string stmtPostDest;
        Instruction* inst;
    };

    void add_seen_cpath_str(std::string cpath_str) {
        seenCPaths.insert(cpath_str);
    }

    void add_borrow(Borrow borrow, bool* adds_cycle) {
        if (does_borrow_add_cycle(borrow, curBI)) {
            if (adds_cycle) {*adds_cycle = true;}
            errs() << "Warning: Adding borrow (from=" << cpath_to_str(borrow.from) << ", to=" << cpath_to_str(borrow.to) << ") adds a cycle!\n";
            errs() << dump_borrows_to_str(curBI);
        } else {
            if (adds_cycle) {*adds_cycle = false;}
        }
        curBI.add(borrow);
    }

    CPathPart get_cpath_part_id_from_str(std::string part_str) {
        assert(!part_str.empty());
        auto it = cpath_part_str_to_id.find(part_str);
        if (it != cpath_part_str_to_id.end()) {
            return it->second;
        } else {
            if (cpath_part_str_to_id.size() == CPathPartMax - 1) {
                // Using CPathPartMax as an actual part ID will cause problems elsewhere.
                errs() << "Too many distinct cpath parts!\n";
                abort();
            }
            // Don't use 0 as a part ID; reserve it as an invalid part ID.
            CPathPart part_id = cpath_part_str_to_id.size() + 1;
            cpath_part_str_to_id.insert({part_str, part_id});
            cpath_part_id_to_str.insert({part_id, part_str});
            return part_id;
        }
    }

    CPath parse_cpath(const std::string& cpath_str) {
        // Example input: "foo.fieldA[0][0].fieldB"
        // Parsed as: {"foo", ".fieldA", "[0]", "[0]", ".fieldB"}
        auto it = cpath_parse_cache.find(cpath_str);
        if (it != cpath_parse_cache.end()) {
            return it->second;
        }
        size_t start = 0;
        size_t i = 0;
        size_t len_cpath_str = cpath_str.size();
        std::vector<CPathPart> ret;
        int starPfxCount = 0;

        while (i < len_cpath_str) {
            if (cpath_str[i] == '*') {
                starPfxCount++;
            } else {
                break;
            }
            ++i;
        }
        start = i;
        while (true) {
            if (i == len_cpath_str || cpath_str[i] == '[' || cpath_str[i] == '.') {
                std::string part_str = cpath_str.substr(start, i - start);
                CPathPart part_id = get_cpath_part_id_from_str(part_str);
                ret.push_back(part_id);
                start = i;
            }
            if (i == len_cpath_str) {
                break;
            }
            ++i;
        }
        assert(get_cpath_part_id_from_str("[0]") == DEREF_PART_ID);
        for (int j=0; j < starPfxCount; j++) {
            ret.push_back(DEREF_PART_ID);
        }

        cpath_parse_cache.insert({cpath_str, ret});

        return ret;
    }

    std::string cpath_to_str(const CPath& cpath, bool use_stars=true) {
        // Count trailing derefs
        size_t trailing_derefs = 0;
        if (use_stars) {
            for (size_t i = cpath.size(); i > 1; i--) {
                if (cpath[i - 1] == DEREF_PART_ID) {
                    trailing_derefs++;
                } else {
                    break;
                }
            }
        }

        // Build string for non-trailing parts (base var + middle parts)
        std::vector<std::string> cpath_str_vec;
        size_t non_trailing_size = cpath.size() - trailing_derefs;
        cpath_str_vec.reserve(non_trailing_size);
        size_t total_size = 0;

        for (size_t i = 0; i < non_trailing_size; i++) {
            auto it = cpath_part_id_to_str.find(cpath[i]);
            std::string part_str;
            if (it == cpath_part_id_to_str.end()) {
                errs() << "Error: Unknown cpath part ID " << cpath[i] << "\n";
                part_str = "???";
            } else {
                part_str = it->second;
            }
            cpath_str_vec.push_back(part_str);
            total_size += part_str.size();
        }

        // Build result with stars prefix
        std::string result;
        result.reserve(trailing_derefs + total_size);

        for (size_t i = 0; i < trailing_derefs; i++) {
            result += "*";
        }
        for (const std::string& s : cpath_str_vec) {
            result += s;
        }

        return result;
    }

    bool cpath_has_deref(CPath cpath) {
        for (CPathPart part : cpath) {
            if (part == DEREF_PART_ID) {
                return true;
            }
        }
        return false;
    }

    std::string get_arg_name_of_base_var(std::string base_var) {
        size_t pos = base_var.find_last_of(':');
        if (pos != std::string::npos) {
            base_var = base_var.substr(pos + 1);  // Everything after the last ':'
            return base_var;
        } else {
            return "";
        }
    }

    std::string get_func_name_of_base_var(std::string base_var) {
        size_t pos = base_var.find_first_of(':');
        if (pos != std::string::npos) {
            base_var = base_var.substr(0, pos);  // Everything before the first ':'
            return base_var;
        } else {
            return "";
        }
    }


    std::string dump_borrows_to_str(const BorrowInfo& bi) {
        std::ostringstream out;

        {
            const auto& s = bi.borrows_by_src;
            if (s.empty()) {
                return out.str(); // nothing to dump
            }

            bool in_group = false;
            CPath current_from;

            for (const Borrow& b : s) {
                if (!in_group || b.from != current_from) {
                    // close previous group
                    if (in_group) {
                        out << "}\n";
                    }
                    // start new group
                    current_from = b.from;
                    in_group = true;
                    out << "  borrow_from[" << cpath_to_str(current_from) << "] = {";
                } else {
                    out << ", ";
                }
                out << cpath_to_str(b.to) << (bi.get_is_must_borrow(b) ? " (!)" : " (?)");
            }
            if (in_group) {
                out << "}\n";
            }
        }

        {
            const auto& s = bi.borrows_by_dest;
            if (s.empty()) {
                return out.str(); // nothing to dump
            }

            bool in_group = false;
            CPath current_to;

            for (const Borrow& b : s) {
                if (!in_group || b.to != current_to) {
                    // close previous group
                    if (in_group) {
                        out << "}\n";
                    }
                    // start new group
                    current_to = b.to;
                    in_group = true;
                    out << "  borrow_to[" << cpath_to_str(current_to) << "] = {";
                } else {
                    out << ", ";
                }
                out << cpath_to_str(b.from) << (bi.get_is_must_borrow(b) ? " (!)" : " (?)");
            }
            if (in_group) {
                out << "}\n";
            }
        }

        return out.str();
    }

    void readArgNameFile(const std::string& filename) {

        // Try opening file
        std::ifstream file(filename);
        // Check if file was successfully opened
        if (!file.is_open()) {
            errs() << "Error: Could not open file '" << filename << "'\n";
            return;
        }

        int line_num = 0;
        std::string line;
        
        while (std::getline(file, line)) {
            line_num++;
            std::istringstream iss(line);
            std::string func_name, cur_arg_name;
            int cur_arg_idx;
            
            if (iss >> func_name >> cur_arg_idx >> cur_arg_name) {
                if (cur_arg_idx < 0) {
                    errs() << "Error on line " << line_num <<  " of " << filename << ": arg index is negative!\n";
                    continue;
                }
                if (cur_arg_idx >= 100) {
                    errs() << "Error on line " << line_num <<  " of " << filename << ": arg index is too big!\n";
                    continue;
                }
                std::vector<std::string>& argNames = funcArgName[func_name];
                argNames.resize(std::max(argNames.size(), (size_t)cur_arg_idx + 1));
                if (!argNames[cur_arg_idx].empty() && argNames[cur_arg_idx] != cur_arg_name) {
                    errs() << "Error in " << filename << ": two definitions for arg " << cur_arg_idx << " of " << func_name << "!\n";
                }
                argNames[cur_arg_idx] = cur_arg_name;
            } else {
                errs() << "Failed to parse line " << line_num << " of " << filename << ": '" << line << "'\n";
            }
        }

        outs() << "Parsed argument names for " << funcArgName.size() << " functions.\n";

        bool dbgPrintArgNames = false;
        if (dbgPrintArgNames) {
            for (const auto& pair : funcArgName) {
                outs() << pair.first << ": ";
                
                const auto& args = pair.second;
                for (size_t i = 0; i < args.size(); ++i) {
                    outs() << i << ":" << args[i];
                    if (i < args.size() - 1) {
                        outs() << ", ";
                    }
                }
                outs() << "\n";
            }
        }
    }

    void readPomPropsFile(const std::string& filename) {
        // Try opening file
        std::ifstream file(filename);
        // Check if file was successfully opened
        if (!file.is_open()) {
            errs() << "Error: Could not open file '" << filename << "'\n";
            return;
        }

        std::string file_basename = basename(filename);

        int line_num = 0;
        std::string line;
        
        std::regex pat_mut_arg("^mut[(]([A-Za-z0-9_]+)[)]");

        std::regex pattern(
            "^([!]?)(\\w+)[(]([*]*)([A-Za-z0-9_:.\\[\\]-]+)(, ([A-Za-z0-9_:.*\\[\\]-]+))?[)]( += +([^#]*))?(#.*)?");
            /*  {neg}{pred}[(]{stars}{     var      }(, {     time      })?[)]( = expr)? */
        while (std::getline(file, line)) {
            line_num++;
            std::smatch matches;
            if (line == "<yaml_file>" || line == "</yaml_file>") {
                continue;
            }
            if (line.length() > 1 && line[0] == '#') {
                // Skip comment line.
                continue;
            }
            if (std::regex_match(line, matches, pattern)) {
                std::string neg   = matches[1].str();
                std::string pred  = matches[2].str();
                std::string stars = matches[3].str();
                std::string var   = matches[4].str();
                std::string time  = matches[6].str(); // matches[5] include the ", " part.
                std::string rhs   = matches[8].str(); // matches[7] include the " = " part.
                std::string func_name = get_func_name_of_base_var(var);
                pomFuncs.insert(func_name);
                pomCPaths.insert(stars+var);
                if (!rhs.empty()) {
                    rtrim_inplace(rhs);
                    if (rhs == std::string("missing!")) {
                        pomMissingErrors.push_back(file_basename + ":" + std::to_string(line_num) + ": missing data for " + pred + "(" + stars + var + ")");
                        continue;
                    }
                    if (pred != std::string("mut")) {
                        errs() << "Error on line " << line_num << " of " << filename << ": only supported for 'mut' so far.\n";
                        continue;
                    }
                    if (!has_suffix(var, "::return")) {
                        errs() << "Error on line " << line_num << " of " << filename << ": only supported for 'func::return' so far.\n";
                        continue;
                    }
                    std::smatch rhs_matches;
                    if (!std::regex_match(rhs, rhs_matches, pat_mut_arg)) {
                        errs() << "Error on line " << line_num << " of " << filename << ": invalid right-hand side (RHS).\n";
                        continue;
                    }
                    std::string arg = rhs_matches[1].str();
                    //errs() << "RHS: mut(" << arg << ")\n";
                    pomDepMutRet[func_name] = arg;
                }
                else if (pred == std::string("borrows_from")) {
                    pomBorrows[func_name][stars+var].insert(time);
                    //errs() << "pomBorrows[\"" << func_name << "\"][\"" << (stars+var) << "\"].insert(\"" << time << "\")\n";
                }
                else {
                    pomPropLines.push_back(line);
                }
            } else {
                errs() << "Error parsing line " << line_num << " of " << filename << "!\n";
            }
        }

        outs() << "Finished reading POM props file " << filename << "\n";
    }

   void computeLiveness(
        Function &F,
        std::map<Value*, size_t> &valueToNum,
        std::map<BasicBlock*, bitvector> &liveAtStart,
        std::map<BasicBlock*, bitvector> &liveAtEnd,
        std::map<Instruction*, std::vector<Value*>> &lastUseIn)
    {
        // valueToNum[val] indicates the position in the bitvector indicating whether val is live.
        // liveAtStart[bb] indicates the values that are live at the start of bb.
        // liveAtEnd[bb] indicates the values that are live at the end of bb.
        // lastUseIn[inst] is list of values that are not used again after inst.

        std::map<std::string, std::vector<Value*>> origVarNameToValues;
        valueToNum.clear();
        liveAtStart.clear();
        liveAtEnd.clear();
        lastUseIn.clear();

        // Step 1: Collect all pointer values and assign them numbers
        std::vector<Value*> pointerValues;
        std::vector<size_t> argsOfPtrType;
        
        // Collect function arguments that are pointers
        for (auto &arg : F.args()) {
            if (arg.getType()->isPointerTy()) {
                valueToNum[&arg] = pointerValues.size();
                pointerValues.push_back(&arg);
                argsOfPtrType.push_back(valueToNum[&arg]);
            }
        }
        
        // Collect instructions that produce pointer values
        for (auto &bb : F) {
            for (auto &inst : bb) {
                if (inst.getType()->isPointerTy()) {
                    valueToNum[&inst] = pointerValues.size();
                    pointerValues.push_back(&inst);
                    std::string origVarName = get_pom_orig_var_name(inst);
                    if (origVarName != "") {
                        origVarNameToValues[origVarName].push_back(&inst);
                    }
                }
            }
        }
        
        size_t numPointerValues = pointerValues.size();
        
        // Step 2: Initialize bitvectors for each basic block
        for (auto &bb : F) {
            liveAtStart[&bb] = bitvector(numPointerValues);
            liveAtEnd[&bb] = bitvector(numPointerValues);
        }

        if (numPointerValues == 0) {
            return; // No pointer values to analyze
        }
        
        // Step 3: Compute gen and kill sets for each basic block
        std::map<BasicBlock*, bitvector> gen, kill;
        for (auto &bb : F) {
            gen[&bb] = bitvector(numPointerValues);
            kill[&bb] = bitvector(numPointerValues);
            
            // Track which values are defined in this block
            bitvector defined(numPointerValues);
            
            // Process instructions in order to compute gen/kill
            for (auto &inst : bb) {
                // First, check operands (uses)
                for (auto &operand : inst.operands()) {
                    Value *val = operand.get();
                    if (val->getType()->isPointerTy()) {
                        auto valueIt = valueToNum.find(val);
                        if (valueIt != valueToNum.end()) {
                            size_t valNum = valueIt->second;
                            // Add to gen set if not already defined in this block
                            if (!defined.read(valNum)) {
                                gen[&bb].write(valNum, true);
                            }
                        }
                    }
                }

                // Keep arguments alive thru the end of the function
                if (ReturnInst *RI = dyn_cast<ReturnInst>(&inst)) {
                    for (size_t arg_var_idx : argsOfPtrType) {
                        gen[&bb].write(arg_var_idx, true);
                    }
                }
                
                // Then, handle definition if this instruction produces a pointer
                if (inst.getType()->isPointerTy()) {
                    auto valueIt = valueToNum.find(&inst);
                    if (valueIt != valueToNum.end()) {
                        size_t valNum = valueIt->second;
                        defined.write(valNum, true);
                        kill[&bb].write(valNum, true);
                        std::string varName = get_pom_orig_var_name(inst);
                        if (varName != "") {
                            for (Value* otherName : origVarNameToValues[varName]) {
                                kill[&bb].write(valueToNum[otherName], true);
                            }
                        }
                    }
                }
            }
            if (DebugLiveness) {
                outs() << "BB " << bb.getName().str() << " GEN: ";
                for (const auto& pair : valueToNum) {
                    Value* val = pair.first;
                    size_t idx = pair.second;
                    if (gen[&bb].read(idx)) {
                        outs() << getCPath(*val) << " ";
                    }
                }
                outs() << "\n";
                outs() << "BB " << bb.getName().str() << " KILL: ";
                for (const auto& pair : valueToNum) {
                    Value* val = pair.first;
                    size_t idx = pair.second;
                    if (kill[&bb].read(idx)) {
                        outs() << getCPath(*val) << " ";
                    }
                }
                outs() << "\n";
            }
        }

        // Step 4: Worklist algorithm for liveness analysis
        std::queue<BasicBlock*> worklist;
        std::set<BasicBlock*> inWorklist; // Track which blocks are already in queue
        for (auto &bb : F) {
            worklist.push(&bb);
            inWorklist.insert(&bb);
        }

        while (!worklist.empty()) {
            BasicBlock *bb = worklist.front();
            worklist.pop();
            inWorklist.erase(bb); // Remove from tracking set
            
            // Compute live-out as union of live-in of all successors
            bitvector liveOut(numPointerValues);
            for (auto *succ : successors(bb)) {
                liveOut |= liveAtStart[succ];
            }
            
            // Store live-out (live at end of block)
            liveAtEnd[bb] = liveOut;
            
            // Compute new live-in: (live-out - kill) ∪ gen
            bitvector newLiveIn = liveOut;
            
            // Subtract kill set: newLiveIn = liveOut - kill
            bitvector killCopy = kill[bb];
            killCopy.negate(); // Invert to create mask
            newLiveIn &= killCopy;
            
            // Add gen set
            newLiveIn |= gen[bb];
            
            // Check if live-in changed
            if (!(newLiveIn == liveAtStart[bb])) {
                liveAtStart[bb] = newLiveIn;
                
                // Add all predecessors to worklist (only if not already present)
                for (auto *pred : predecessors(bb)) {
                    if (inWorklist.find(pred) == inWorklist.end()) {
                        worklist.push(pred);      // Add to end of queue
                        inWorklist.insert(pred);  // Track that it's in queue
                    }
                }
            }
        }

        // Step 5: Compute lastUseIn by analyzing last uses within each block
        for (auto &bb : F) {
            // Get live-out for this block (already computed and stored)
            bitvector currentlyLive = liveAtEnd[&bb];
            
            // Process instructions in reverse order
            for (auto I = bb.rbegin(); I != bb.rend(); ++I) {
                Instruction *Inst = &*I;
                std::vector<Value*> lastUses;
                
                // If this instruction defines a value, it's no longer live after this point
                if (Inst->getType()->isPointerTy()) {
                    size_t idx = valueToNum[Inst];
                    currentlyLive.write(idx, false);
                }
                
                // Check each operand to see if this is its last use
                for (auto &Op : Inst->operands()) {
                    Value *V = Op.get();
                    if (valueToNum.count(V)) {
                        size_t idx = valueToNum[V];
                        if (!currentlyLive.read(idx) && !isa<Argument>(V)) {
                            // This is the last use of V in this block
                            lastUses.push_back(V);
                        }
                        // V becomes live because it's used here
                        currentlyLive.write(idx, true);
                    }
                }
                
                // Store last uses if any
                if (!lastUses.empty()) {
                    lastUseIn[Inst] = lastUses;
                }
            }
        }
    }

    void do_liveness(Function &F) {
        bool dbg = DebugLiveness;
        std::map<BasicBlock*, bitvector> liveAtStartBV;
        std::map<BasicBlock*, bitvector> liveAtEndBV;
        std::map<Value*, size_t> valueToNum; 
        std::vector<Value*> numToValue;

        computeLiveness(F, valueToNum, liveAtStartBV, liveAtEndBV, lastUseIn);

        if (dbg) outs() << "BEGIN number-to-value mapping for " << F.getName().str() << "\n";
        numToValue.resize(valueToNum.size(), nullptr);
        for (const auto& pair : valueToNum) {
            if (dbg) outs() << "  " << pair.second << ": " << getCPath(*pair.first) << "\n  ";
            if (dbg) pair.first->dump();
            numToValue[pair.second] = pair.first;
        }
        if (dbg) outs() << "END number-to-value mapping\n";

        if (dbg) outs() << "BEGIN liveAtStart for " << F.getName().str() << "\n";
        for (const auto& pair : liveAtStartBV) {
            BasicBlock* bb = pair.first;
            bitvector bv = pair.second;
            std::set<Value*> liveVals;
            if (dbg) outs() << "  " << bb->getName().str() << ": ";
            for (int i=0; i < bv.size(); i++) {
                if (bv.read(i)) {
                    Value* val = numToValue[i];
                    liveVals.insert(val);
                    if (dbg) outs() << getCPath(*val) << " ";
                }
            }
            if (dbg) outs() << "\n";
            liveAtStartSet[bb] = liveVals;
        }
        if (dbg) outs() << "END liveAtStart\n";

        if (dbg) outs() << "BEGIN liveAtEnd for " << F.getName().str() << "\n";
        for (const auto& pair : liveAtEndBV) {
            BasicBlock* bb = pair.first;
            bitvector bv = pair.second;
            std::set<Value*> liveVals;
            if (dbg) outs() << "  " << bb->getName().str() << ": ";
            for (int i=0; i < bv.size(); i++) {
                if (bv.read(i)) {
                    Value* val = numToValue[i];
                    liveVals.insert(val);
                    if (dbg) outs() << getCPath(*val) << " ";
                }
            }
            if (dbg) outs() << "\n";
            liveAtEndSet[bb] = liveVals;
        }
        if (dbg) outs() << "END liveAtEnd\n";

        if (dbg) {
            outs() << "BEGIN lastUseIn for " << F.getName().str() << "\n";
            for (const auto& pair : lastUseIn) {
                Instruction* inst = pair.first;
                std::vector<Value*> killedVars = pair.second;
                if (killedVars.size() > 0) {
                    errs() << "  I" << getInstructionId(*inst) << ": ";
                    inst->dump();
                    errs() << "  I" << getInstructionId(*inst) << " killed vars: ";
                    for (Value* var : killedVars) {
                        errs() << getCPath(*var) << " ";
                    }
                    errs() << "\n";
                }
            }
            outs() << "END lastUseIn\n\n";
        }
    }

    void setBorrowInfoAtBasicBlockStart(BasicBlock* curBasicBlock) {
        
        curBI = BorrowInfo();

        // Collect incoming BorrowInfo from predecessor basic blocks
        std::vector<BorrowInfo*> incoming;
        for (BasicBlock* pred_bb : predecessors(curBasicBlock)) {
            auto it = borrows_at_end_of_bb.find(pred_bb);
            if (it != borrows_at_end_of_bb.end()) {
                incoming.push_back(&it->second);
            }
        }

        // Union all borrows from incoming
        for (BorrowInfo* bi : incoming) {
            for (const Borrow& b : bi->borrows_by_src) {
                curBI.borrows_by_src.insert(b);
                curBI.borrows_by_dest.insert(b);
            }
        }

        // Collect all base_vars from incoming's is_dyn_exclusive keys
        std::set<CPathPart> all_base_vars;
        for (BorrowInfo* bi : incoming) {
            for (const auto& kv : bi->is_dyn_exclusive) {
                all_base_vars.insert(kv.first);
            }
        }

        // For each base_var, AND together is_dyn_exclusive values (default=true)
        for (CPathPart base_var : all_base_vars) {
            bool result = true;
            for (BorrowInfo* bi : incoming) {
                auto it = bi->is_dyn_exclusive.find(base_var);
                bool value = (it != bi->is_dyn_exclusive.end()) ? it->second : true;
                result = result && value;
            }
            curBI.is_dyn_exclusive[base_var] = result;
        }

        // For each borrow in curBI, AND together is_must_borrow values (default=false)
        for (const Borrow& b : curBI.borrows_by_src) {
            bool result = true;
            for (BorrowInfo* bi : incoming) {
                auto it = bi->is_must_borrow.find(b);
                bool value = (it != bi->is_must_borrow.end()) ? it->second : false;
                result = result && value;
            }
            curBI.is_must_borrow[b] = result;
        }

        // Union all waiting_to_die from incoming
        for (BorrowInfo* bi : incoming) {
            for (const CPath& cpath : bi->waiting_to_die) {
                curBI.waiting_to_die.insert(cpath);
            }
        }

    }

    void handleCPathBecomingDead(const CPath& dead_cpath, bool force_kill=false) {

        if (!isDryRun) {
            //errs() << "[Func " << curFunc->getName().str() << "] [I" << curInstructionID << "] Removing dead borrows from " << cpath_to_str(dead_cpath) << "\n";
        }

        // Find all borrows where 'to' starts with dead_cpath
        std::vector<Borrow> borrows_to_dead;
        {
            CPath dead_cpath_plus_max = dead_cpath;
            dead_cpath_plus_max.push_back(CPathPartMax);
            const Borrow lower = {.to = dead_cpath, .from = {0}};
            const Borrow upper = {.to = dead_cpath_plus_max, .from = {0}};
            auto itLo = curBI.borrows_by_dest.lower_bound(lower);
            auto itHi = curBI.borrows_by_dest.lower_bound(upper);
            for (auto it = itLo; it != itHi; ++it) {
                borrows_to_dead.push_back(*it);
            }
        }
        
        // Find all borrows where 'from' starts with dead_cpath
        std::vector<Borrow> borrows_from_dead;
        {
            CPath dead_cpath_plus_max = dead_cpath;
            dead_cpath_plus_max.push_back(CPathPartMax);
            const Borrow lower = {.to = {0}, .from = dead_cpath};
            const Borrow upper = {.to = {0}, .from = dead_cpath_plus_max};
            auto itLo = curBI.borrows_by_src.lower_bound(lower);
            auto itHi = curBI.borrows_by_src.lower_bound(upper);
            for (auto it = itLo; it != itHi; ++it) {
                borrows_from_dead.push_back(*it);
            }
        }
        
        if (borrows_from_dead.size() == 0 && borrows_to_dead.size() == 0) {
            // Nothing to do, so just leave.
            return;
        }

        Value* dead_LLVM = cpathToValue[cpath_part_id_to_str[dead_cpath[0]]];
        bool is_phi = dead_LLVM != nullptr && isa<PHINode>(dead_LLVM);
        bool is_alloca = dead_LLVM != nullptr && isa<AllocaInst>(dead_LLVM);
        if (is_alloca && borrows_to_dead.size() == 0 && !is_phi && !force_kill) {
            if (borrows_from_dead.size() > 0) {
                if (dead_cpath.size() == 1) {
                    CPathPart renamed_dead_base_var = rename_dead_basevar_in_borrows(dead_cpath[0]);
                    curBI.waiting_to_die.insert((CPath){renamed_dead_base_var});
                } else {
                    curBI.waiting_to_die.insert(dead_cpath);
                }
            }
            return;
        }
        
        // Helper to get is_must_borrow value with default of false
        auto get_is_must_borrow = [this](const Borrow& b) -> bool {
            auto it = curBI.is_must_borrow.find(b);
            return (it != curBI.is_must_borrow.end()) ? it->second : false;
        };
        
        // Splice borrows and create new ones
        std::vector<std::pair<Borrow, bool>> new_borrows; // pair of (borrow, is_must)
        std::vector<std::pair<Borrow, bool>> rejected_borrows; // pair of (borrow, is_must)
        
        for (const auto& borrow_toA_fromB : borrows_to_dead) {
            for (const auto& borrow_toC_fromD : borrows_from_dead) {
                // borrow_toA_fromB: to=(dead_cpath + suffixA), from=(varB + suffixB)
                // borrow_toC_fromD: to=(varC + suffixC),       from=(dead_cpath + suffixD)
                
                // Extract CPaths from borrows
                const CPath& path_A = borrow_toA_fromB.to;
                const CPath& path_B = borrow_toA_fromB.from;
                const CPath& path_C = borrow_toC_fromD.to;
                const CPath& path_D = borrow_toC_fromD.from;

                // Extract suffixes
                CPath suffixA(path_A.begin() + dead_cpath.size(), path_A.end());
                CPath suffixD(path_D.begin() + dead_cpath.size(), path_D.end());
                
                bool must1 = get_is_must_borrow(borrow_toA_fromB);
                bool must2 = get_is_must_borrow(borrow_toC_fromD);
                bool new_must = must1 && must2;
                
                // Case 1: suffixA starts with suffixD
                // Add Borrow(to=(varC + suffixC + suffixA.remove_prefix(suffixD)), from=(varB + suffixB))
                if (starts_with(suffixA, suffixD)) {
                    CPath new_to = path_C;
                    new_to.insert(new_to.end(), suffixA.begin() + suffixD.size(), suffixA.end());
                    Borrow new_borrow = {.to = new_to, .from = path_B};
                    if (new_borrow.to != new_borrow.from) {
                        new_borrows.push_back({new_borrow, new_must});
                    } else {
                        rejected_borrows.push_back({new_borrow, new_must});
                    }
                }
                
                // Case 2: suffixD starts with suffixA
                // Add Borrow(to=(varC + suffixC), from=(varB + suffixB + suffixD.remove_prefix(suffixA)))
                if (starts_with(suffixD, suffixA)) {
                    CPath new_from = path_B;
                    new_from.insert(new_from.end(), suffixD.begin() + suffixA.size(), suffixD.end());
                    Borrow new_borrow = {.to = path_C, .from = new_from};
                    if (new_borrow.to != new_borrow.from) {
                        new_borrows.push_back({new_borrow, new_must});
                    } else {
                        rejected_borrows.push_back({new_borrow, new_must});
                    }
                }
            }
            for (const auto& borrow_toD_fromC : borrows_to_dead) {
                // borrow_toA_fromB: to=(dead_cpath + suffixA), from=(varB + suffixB)
                // borrow_toD_fromC: to=(dead_cpath + suffixD), from=(varC + suffixC)
                
                // Extract CPaths from borrows
                const CPath& path_A = borrow_toA_fromB.to;
                const CPath& path_B = borrow_toA_fromB.from;
                const CPath& path_C = borrow_toD_fromC.from;
                const CPath& path_D = borrow_toD_fromC.to;

                // Extract suffixes
                CPath suffixA(path_A.begin() + dead_cpath.size(), path_A.end());
                CPath suffixD(path_D.begin() + dead_cpath.size(), path_D.end());
                
                bool must1 = get_is_must_borrow(borrow_toA_fromB);
                bool must2 = get_is_must_borrow(borrow_toD_fromC);
                bool new_must = must1 && must2;

                if (suffixD.size() > suffixA.size() && starts_with(suffixD, suffixA)) {
                    // Add Borrow(to=(varB + suffixB + suffixD.remove_prefix(suffixA)), from=(varC + suffixC))
                    CPath new_to = path_B;
                    new_to.insert(new_to.end(), suffixD.begin() + suffixA.size(), suffixD.end());
                    Borrow new_borrow = {.to = new_to, .from = path_C};
                    if (new_borrow.to != new_borrow.from) {
                        new_borrows.push_back({new_borrow, new_must});
                    }
                }
            }
        }

        std::string dead_cpath_str = cpath_to_str(dead_cpath);

        if (rejected_borrows.size() > 0) {
            errs() << dump_borrows_to_str(curBI);
            errs() << "Rejected borrows on death of " << dead_cpath_str << " :\n";
            for (auto const& [borrow, is_must] : rejected_borrows) {
                errs() << " - Borrow from " << cpath_to_str(borrow.from) << " to " << cpath_to_str(borrow.to) << "\n";
            }
        }
        
        // Track base vars of deleted borrows for is_dyn_exclusive update
        std::set<CPathPart> affected_base_vars;
        affected_base_vars.insert(dead_cpath[0]);
        for (const auto& b : borrows_to_dead) {
            if (!b.from.empty()) {
                affected_base_vars.insert(b.from[0]);
            }
        }
        
        // Delete borrows where to or from starts with dead_cpath
        for (const std::vector<Borrow>& borrows_to_or_from_dead : {borrows_to_dead, borrows_from_dead}) {
            for (const auto& b : borrows_to_or_from_dead) {
                //errs() << "[dead cpath " << dead_cpath_str << "] Deleting borrow (from=" << cpath_to_str(b.from) << ", to=" << cpath_to_str(b.to) << ")\n";
                curBI.borrows_by_src.erase(b);
                curBI.borrows_by_dest.erase(b);
                curBI.is_must_borrow.erase(b);
            }
        }
        
        // Add new borrows
        for (const auto& pair : new_borrows) {
            const Borrow& b = pair.first;
            bool is_must = pair.second;
            bool makes_cycle = false;
            add_borrow(b, &makes_cycle);
            if (makes_cycle) {
                errs() << "(from handleCPathBecomingDead on " << cpath_to_str(dead_cpath) << ")\n\n";
            }
            //errs() << "[dead cpath " << dead_cpath_str << "] Adding borrow (from=" << cpath_to_str(b.from) << ", to=" << cpath_to_str(b.to) << ")\n";
            curBI.is_must_borrow[b] = is_must;
        }
        
        // Update is_dyn_exclusive for affected base vars
        // If count of borrows rooted at base_var is now 1, set is_dyn_exclusive[base_var] = true
        for (CPathPart base_var : affected_base_vars) {
            int count = 0;
            CPath base_cpath = {base_var};
            CPath base_cpath_plus_max = {base_var, CPathPartMax};
            const Borrow lower = {.to = {0}, .from = base_cpath};
            const Borrow upper = {.to = {0}, .from = base_cpath_plus_max};
            auto itLo = curBI.borrows_by_src.lower_bound(lower);
            auto itHi = curBI.borrows_by_src.lower_bound(upper);
            for (auto it = itLo; it != itHi; ++it) {
                count++;
            }
            
            if (count == 1) {
                curBI.is_dyn_exclusive[base_var] = true;
            } else if (count == 0) {
                curBI.is_dyn_exclusive.erase(base_var);
            }
        }
    }   

    void check_on_waiting_to_die() {
        while (true) {
            std::vector<CPath> killed;
            for (CPath cpath : curBI.waiting_to_die) {
                if (hasBorrowsFrom(cpath)) {
                    continue;
                }
                handleCPathBecomingDead(cpath);
                killed.push_back(cpath);
            }
            for (CPath cpath : killed) {
                curBI.waiting_to_die.erase(cpath);
            }
            if (killed.size() == 0) {
                break;
            }
        }
    }

    bool has_nondummy_uses(Instruction* inst) {
        for (auto &U : inst->uses()) {
            User *Usr = U.getUser();
            if (Instruction *UserInst = dyn_cast<Instruction>(Usr)) {
                if (!isDbgIntrinsic(*UserInst)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool runOnModule(Module &M) {
        
        for (int i=0; ; i++) {
            hasUpdatedBorrows = false;
            for (Function &F : M) {
                if (F.isDeclaration()) {
                    continue;
                }
                runOnFunction(F);
            }
            if (!hasUpdatedBorrows || !WholeProg) {
                break;
            }
            if (i >= 100) {
                errs() << "Error: maximum iterations exceeded; exiting!\n";
                break;
            }
            if (outFile.is_open()) {
                outFile.close();
            }
            if (numirFile.is_open()) {
                numirFile.close();
            }
        }
        if (CheckPomComplete) {
            outFile << "<common>\n";
            for (std::string& missing : pomMissingErrors) {
                outFile << "false # " << missing << "\n";
            }
            outFile << "</common>\n";
        }
        outFile << "<yaml_file>\n";
        for (std::string line : pomPropLines) {
            outFile << line << "\n";
        }
        outFile << "</yaml_file>\n";
        if (seen_unsupported.size()) {
            outFile << "# UNSUPPORTED_FEATURES: [";
            for (const std::string& feat : seen_unsupported) {
                outFile << " " << feat;
            }
            outFile << " ]\n";
        }

        bool modified = true;
        return modified;
    }
    
    bool runOnFunction(Function &F) {
        if (OutputFile.empty()) {
            errs() << "Error: No output file specified. Use -output <filename>\n";
            return false;
        }
        
        bool already_opened = outFile.is_open();
        if (!already_opened) {
            outFile.open(OutputFile, std::ios::out);
        }
        if (!outFile.is_open()) {
            errs() << "Error: Cannot open output file " << OutputFile << "\n";
            return false;
        }
        if (!already_opened) {
            outFile << "<common>\n";
            outFile << "!false\n";
            outFile << "</common>\n\n";
        }
        
        if (!NumberedIR.empty()) {
            if (!numirFile.is_open()) {
                numirFile.open(NumberedIR, std::ios::out);
            }
            if (!numirFile.is_open()) {
                errs() << "Error: Cannot open numbered-IR output file " << NumberedIR << "\n";
            }
        }

        if (!alreadyInit) {
            alreadyInit = true;
            if (!ArgNameFile.empty()) {
                readArgNameFile(ArgNameFile);
            }
            if (!PomPropsFile.empty()) {
                readPomPropsFile(PomPropsFile);
            }
        }

        if (DisableNullCheck2) {
            DisableNullCheck = true;
        }

        CPathPart deref_part_id = this->get_cpath_part_id_from_str("[0]"); // This inserts it.
        if (deref_part_id != DEREF_PART_ID) {
            abort();
        }

        this->curFunc = &F;
        if (!GloUniq) {
            nextInstId = 1;
            nextVarId = 1;
        }

        outFile << "<function name=\"" << F.getName().str() << "\">\n";
        if (!NumberedIR.empty()) {
            numirFile << "function " << F.getName().str() << " {\n";
        }
        
        std::vector<PHINode*> all_phis;

        // Build debug info mapping first
        buildDebugInfoMapping(F);

        std::vector<DILocalVariable*> arg_to_DI_var = std::vector<DILocalVariable*>(F.arg_size() + 1, nullptr);
        
        // Assign IDs to all instructions
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (!isDbgIntrinsic(I)) {
                    if (instToId.count(&I) == 0) {
                        instToId[&I] = nextInstId++;
                    }
                }
                if (isDbgIntrinsic(I)) {
                    DILocalVariable* DI_var = nullptr;
                    Value* prog_var = nullptr;
                    if (const auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {
                        prog_var = DDI->getAddress();
                        DI_var = DDI->getVariable();
                    } else if (const auto *DVI = dyn_cast<DbgValueInst>(&I)) {
                        prog_var = DVI->getValue();
                        DI_var = DVI->getVariable();
                    }
                    if (DI_var && prog_var && isa<Argument>(prog_var)) {
                        int argIx = dyn_cast<Argument>(prog_var)->getArgNo();
                        arg_to_DI_var[argIx] = DI_var;
                    }
                }
                if (auto *phi = dyn_cast<PHINode>(&I)) {
                    all_phis.push_back(phi);
                }
            }
        }
        
        // Add metadata to phi instructions
        {
            bool isDirty = true;
            std::vector<PHINode*> curList = all_phis;
            std::vector<PHINode*> nextList;

            //outs() << "Doing phis for function " << F.getName().str() << "\n";
            while (isDirty) {
                isDirty = false;
                for (PHINode* phi : curList) {
                    if (!phi->getType()->isPointerTy()) {
                        continue;
                    }
                    if (get_phi_origin_metadata(phi) == std::string("indirect_call_expansion")) {
                        continue;
                    }
                    std::set<std::string> varNames;
                    DILocalVariable* chosenDIVar = nullptr;
                    for (Value *incomingValue : phi->incoming_values()) {
                        if (Instruction* inst = dyn_cast<Instruction>(incomingValue)) {
                            DILocalVariable* curDIVar = get_pom_orig_var_DIVar(*inst);
                            if (curDIVar) {
                                chosenDIVar = curDIVar;
                                std::string curName = curDIVar->getName().str();
                                varNames.insert(curName);
                            } else {
                                std::string curName = get_pom_orig_var_name(*inst);
                                if (curName != std::string("")) {
                                    varNames.insert(curName);
                                }
                            }
                        }
                    }
                    if (varNames.size() == 1) {
                        if (chosenDIVar) {
                            isDirty = true;
                            //outs() << "Setting metadata for this phi: \n";
                            //phi->dump();
                            phi->setMetadata("pom_orig_var", chosenDIVar);
                        }
                    } else if (varNames.size() > 1) {
                        errs() << "Error: function " << F.getName().str() << ": phi node has incoming values from more than one original variable!\n";
                        phi->dump();
                    } else {
                        nextList.push_back(phi);
                        //outs() << "Nothing for this phi: \n";
                        //phi->dump();
                    }
                }
                curList = nextList;
                nextList.clear();
            }
            if (curList.size() > 0 && !NoWarnPhis) {
                errs() << "Warning: Missing metadata for these phis in function " << F.getName().str() << ":\n";
                for (PHINode* phi : curList) {
                    phi->dump();
                }
            }
        }
        
        bool is_first_time = false;

        // Rename instruction results
        if (!already_renamed_func_values.count(curFunc)) {
            is_first_time = true;
            already_renamed_cur_func_values = false;
            //errs() << "Renaming instruction results for function...\n";
            std::vector<std::pair<Value*, std::string>> new_value_names;
            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (!isDbgIntrinsic(I)) {
                        if (!I.getType()->isVoidTy()) {
                            new_value_names.push_back({&I, getCPath(I)});
                        }
                        //for (const llvm::Use &U : I.operands()) {
                        //    Value* operand = U.get();
                        //    new_value_names.push_back({operand, getCPath(*operand)});
                        //}
                    }
                }
            }
            for (auto inst_and_name : new_value_names) {
                Value* inst = inst_and_name.first;
                std::string new_name = inst_and_name.second;
                inst->setName(new_name);
                //errs() << new_name << ": ";
                //inst->dump();
            }
            already_renamed_func_values.insert(curFunc);
            //errs() << "Done renaming instruction results for function.\n";
        } else {
            //errs() << "Already renamed instruction results for function " << curFunc->getName().str() << "\n";
        }
        already_renamed_cur_func_values = true;


        if (!DisableLiveness) {
            do_liveness(F);
        }

        // Handle NULL literals
        {
            llvm::Instruction* firstInst = get_first_real_inst_of_BB(F.getEntryBlock());
            std::string start = getStatementLabel(*firstInst, "pre");
            outFile << "null(NULL_CONST, " << start << ")\n";
            outFile << "!good(NULL_CONST, " << start << ")\n";
            outFile << "!zombie(NULL_CONST, " << start << ")\n";
            add_seen_cpath_str(std::string("NULL_CONST"));
        }

        // Process instructions and generate constraints
        // First pass is a dry run to populate seenCPaths without generating output
        // Final pass generates the actual constraints
        int prevNumCPaths = -1;
        isDryRun = true;
        bool changed_borrows = false;
        while (isDryRun == true) {
            changed_borrows = false;
            if (prevNumCPaths == seenCPaths.size() || changed_borrows) {
                isDryRun = false;
            }
            prevNumCPaths = seenCPaths.size();

            std::vector<Borrow> arg_dummy_borrows;

            std::string funcName = F.getName().str();
            for (int i = 0; i < F.arg_size(); i++) {
                Argument* arg = F.getArg(i);
                std::string argName = getArgName(arg);
                argNameSet.insert(argName);
                std::string func__args__arg = funcName + "::args::" + arg->getName().str();
                DILocalVariable* DI_var = arg_to_DI_var[i];
                int ptr_level = 0;
                if (DI_var) {
                    ptr_level = countPointerIndirectionFromDI(DI_var->getType());
                } else {
                    if (!isDryRun) {
                        errs() << "Warning: missing debug info for argument " << i << " of function " << funcName << "!\n";
                    }
                    ptr_level = (arg->getType()->isPointerTy() ? 1 : 0);
                }
                if (ptr_level > 0) {
                    add_seen_cpath_str(argName);
                    if (!isDryRun) {
                        outFile << "# Argument " << arg->getName().str() << "\n";
                        outFile << "!responsible(" << argName << ") | responsible(" << func__args__arg << ")\n";
                        outFile << "responsible(" << argName << ") | !responsible(" << func__args__arg << ")\n";
                        outFile << "!mut(" << argName << ") | mut(" << func__args__arg << ")\n";
                        outFile << "mut(" << argName << ") | !mut(" << func__args__arg << ")\n";
                        outFile << "!good(" << argName << ", start) | good(" << func__args__arg << ", start)\n";
                        outFile << "good(" << argName << ", start) | !good(" << func__args__arg << ", start)\n";
                        outFile << "!null(" << argName << ", start) | null(" << func__args__arg << ", start)\n";
                        outFile << "null(" << argName << ", start) | !null(" << func__args__arg << ", start)\n";
                        outFile << "!zombie(" << argName << ", start) | zombie(" << func__args__arg << ", start)\n";
                        outFile << "zombie(" << argName << ", start) | !zombie(" << func__args__arg << ", start)\n";
                        if (is_first_time) {
                            if (!pomCPaths.count(func__args__arg)) {
                                pomMissingErrors.push_back("Missing in '.pom.yml' file: " + func__args__arg);
                            }
                        }
                    }
                }
                std::string starPfx = "*";
                std::string fakeDerefSfx = "<0>";
                for (int i=1; i < ptr_level; i++) {
                    add_seen_cpath_str(starPfx + argName);
                    arg_dummy_borrows.push_back((Borrow){
                        .to = parse_cpath(starPfx + argName),
                        .from = parse_cpath(argName + ":@orig" + fakeDerefSfx)
                    });
                    if (!isDryRun) {
                        outFile << "!responsible(" << starPfx << argName << ") | responsible(" << starPfx << func__args__arg << ")\n";
                        outFile << "responsible(" << starPfx << argName << ") | !responsible(" << starPfx << func__args__arg << ")\n";
                        outFile << "!mut(" << starPfx << argName << ") | mut(" << starPfx << func__args__arg << ")\n";
                        outFile << "mut(" << starPfx << argName << ") | !mut(" << starPfx << func__args__arg << ")\n";
                        for (std::string timePt : {"start", "end"}) {
                            outFile << "!good(" << starPfx << argName << ", " << timePt << ") | good(" << starPfx << func__args__arg << ", " << timePt << ")\n";
                            outFile << "good(" << starPfx << argName << ", " << timePt << ") | !good(" << starPfx << func__args__arg << ", " << timePt << ")\n";
                            outFile << "!null(" << starPfx << argName << ", " << timePt << ") | null(" << starPfx << func__args__arg << ", " << timePt << ")\n";
                            outFile << "null(" << starPfx << argName << ", " << timePt << ") | !null(" << starPfx << func__args__arg << ", " << timePt << ")\n";
                            outFile << "!zombie(" << starPfx << argName << ", " << timePt << ") | zombie(" << starPfx << func__args__arg << ", " << timePt << ")\n";
                            outFile << "zombie(" << starPfx << argName << ", " << timePt << ") | !zombie(" << starPfx << func__args__arg << ", " << timePt << ")\n";
                        }
                        if (is_first_time) {
                            if (!pomCPaths.count(starPfx + func__args__arg)) {
                                pomMissingErrors.push_back("Missing in '.pom.yml' file: " + starPfx + func__args__arg);
                            }
                        }
                    }
                    starPfx += "*";
                    fakeDerefSfx += "<0>";
                }
                // In case the program uses casts to get further levels of indirection,
                // treat the next level of indirection as starting in a garbage state.
                {
                    std::string timePt = "start";
                    outFile << "zombie(" << starPfx << argName << ", " << timePt << ") # Try to flag exceeding declared pointer depth\n";
                }
            }
            if (pomDepMutRet.count(curFunc->getName().str())) {
                outFile << "false # Cannot yet verify user-defined functions with dependent mutability\n";
            }
            
            for (BasicBlock &BB : F) {
                Instruction *prevInst = nullptr;
                if (!isDryRun) {
                    if (!NumberedIR.empty()) {
                        numirFile << "BB " << BB.getName().str() << ":\n";
                    }
                }

                if (!isDryRun) {
                    outFile << "\n<BasicBlock name=\"" << BB.getName().str() << "\">\n";
                }

                if (!DisableBorrowChecks) {
                    setBorrowInfoAtBasicBlockStart(&BB);
                }
                if (&BB == &curFunc->getEntryBlock()) {
                    for (Borrow& borrow : arg_dummy_borrows) {
                        add_borrow(borrow, nullptr);
                        curBI.is_must_borrow[borrow] = true;
                    }
                }

                bool bb_ends_in_unreachable = isa<UnreachableInst>(BB.getTerminator());

                /* Handle variables that are killed in some but not all predecessor basic blocks */
                std::set<Value*> liveAtAnyPred;
                //if (!isDryRun) {
                //    errs() << "Looking at predecessors of BB " << BB.getName().str() << "\n";
                //}
                for (BasicBlock* pred_bb : predecessors(&BB)) {
                    // We need to include the variables live at the pred BB start to handle realloc weirdness.
                    std::set<Value*>& liveAtCurPred = liveAtStartSet[pred_bb];
                    liveAtAnyPred.insert(liveAtCurPred.begin(), liveAtCurPred.end());

                    std::set<Value*>& liveAtCurPredEnd = liveAtEndSet[pred_bb];
                    liveAtAnyPred.insert(liveAtCurPredEnd.begin(), liveAtCurPredEnd.end());
                }
                currentLiveVars = liveAtStartSet[&BB];
                
                // if (!DisableBorrowChecks) {
                //     std::set<CPathPart> all_base_vars;
                //     for (const auto& kv : curBI.is_dyn_exclusive) {
                //         all_base_vars.insert(kv.first);
                //     }
                //     for (CPathPart base_var_id : all_base_vars) {
                //         std::string& base_var_name = cpath_part_id_to_str[base_var_id];
                //         Value* base_var_LLVM = cpathToValue[base_var_name];
                //         if (currentLiveVars.count(base_var_LLVM) == 0) {
                //             handleCPathBecomingDead((CPath){base_var_id});
                //         }
                //     }
                // }

                //if (!DisableBorrowChecks && !isDryRun) {
                //    errs() << dump_borrows_to_str(curBI);
                //}
                for (Value* var : liveAtAnyPred) {
                    std::string cpath = getCPath(*var);
                    if (currentLiveVars.count(var) == 1 || isa<Argument>(var)) {
                        if (!isDryRun) {
                            //errs() << "Still alive at start of BB " << BB.getName().str() << ": " << cpath << "\n";
                        }
                        continue;
                    }
                    if (!isDryRun) {
                        //errs() << "Info: Dead var at start of BB " << BB.getName().str() << ": " << cpath << "\n";
                    }
                    if (!DisableBorrowChecks) {
                        handleCPathBecomingDead(parse_cpath(cpath));
                    }
                    if (!DisableMemLeak && !bb_ends_in_unreachable) {
                        if (!isDryRun) {
                            Instruction* first_inst = get_first_real_inst_of_BB(BB);
                            std::string bbPre = getStatementLabel(*first_inst, "pre");
                            // responsible(x) -> !good(x, S-pre)
                            outFile << "!responsible(" << cpath << ") | !good(" << cpath << ", " << bbPre << ") # killed_var\n";
                            if (isa<AllocaInst>(var) && seenCPaths.count(std::string("*") + cpath)) {
                                outFile << "!responsible(*" << cpath << ") | !good(*" << cpath << ", " << bbPre << ") # Death of good resp ptr causes a mem leak\n"; /* pri_cat=mem_leak */
                            }
                        }
                    }
                }
                check_on_waiting_to_die();
                //if (!DisableBorrowChecks && !isDryRun) {
                //    errs() << dump_borrows_to_str(curBI);
                //}

                std::vector<CPath> postponed_kills;
                for (Instruction &I : BB) {
                    if (isDbgIntrinsic(I)) {
                        continue;
                    }
                    
                    // Generate intra-basic-block flow constraints
                    if (prevInst != nullptr) {
                        if (!isDryRun) {
                            outFile << "# Flow to next instruction\n";
                        }
                        generateFlowConstraints(*prevInst, I);
                        if (!isDryRun) {
                            outFile << "</instruction>\n";
                        }
                    }
                    
                    if (!isDryRun) {
                        //outFile << "# Instruction '" << std::string(I.getOpcodeName()) << "': " << getStatementLabel(I, "") << "\n";
                        outFile << "\n<instruction opcode=\"" << std::string(I.getOpcodeName()) << "\" ";
                        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                            if (Function *F = getCalledFunc(CI)) {
                                outFile << "callee=\"" << F->getName().str() << "\" ";
                            }
                        }
                        outFile << "loc=\"" << getStatementLabel(I, "") << "\">\n";
                        if (!NumberedIR.empty()) {
                            std::string instStr;
                            llvm::raw_string_ostream rso(instStr);
                            I.print(rso);
                            static const std::regex pattern(", !dbg ![0-9]+");
                            instStr = std::regex_replace(instStr, pattern, "");
                            //std::string varNameEq;
                            //if (has_nondummy_uses(&I)) {
                            //    varNameEq = getCPath(I) + " = ";
                            //}
                            //if (varNameEq.length() < 14) {
                            //    varNameEq.append(14 - varNameEq.length(), ' ');
                            //}
                            std::string line_num_str;
                            if (const DebugLoc &DL = I.getDebugLoc()) {
                                int line_num = DL.getLine();
                                if (line_num != 0) {
                                    line_num_str = "  // Line " + std::to_string(line_num);
                                }
                            }
                            //numirFile << "  I" << instToId[&I] << ":\t" << varNameEq << instStr << line_num_str << "\n";
                            numirFile << "  I" << instToId[&I] << ":\t" << instStr << line_num_str << "\n";
                        }
                    }
                    processInstruction(I);
                    this->curInstruction = nullptr;
                    this->curInstructionID = 0;
                    //if (!isDryRun) {
                    //    errs() << "Finished I" << instToId[&I] << ".\n";
                    //}
                    if (!DisableLiveness) {
                        auto itLastUse = lastUseIn.find(&I);
                        if (itLastUse != lastUseIn.end()) {
                            if (!DisableMemLeak) {
                                if (!isDryRun) {
                                    outFile << "# Killed variables\n";
                                }
                            }
                            for (Value* killedVar : itLastUse->second) {
                                std::string cpath = getCPath(*killedVar);
                                if (!DisableBorrowChecks) {
                                    bool is_br_cmp = isa<ICmpInst>(&I) && isa<BranchInst>(I.getNextNonDebugInstruction());
                                    if (is_br_cmp) {
                                        postponed_kills.push_back(parse_cpath(cpath));
                                    } else {
                                        handleCPathBecomingDead(parse_cpath(cpath));
                                    }
                                }
                                bool is_arg_to_realloc = argsToRealloc.count(cpath);
                                if (!is_arg_to_realloc) {
                                    currentLiveVars.erase(killedVar);
                                }
                                if (!DisableMemLeak && !bb_ends_in_unreachable && !is_arg_to_realloc) {
                                    if (!isDryRun) {
                                        std::string stmtPost = getStatementLabel(I, "post");
                                        outFile << "!responsible(" << cpath << ") | !good(" << cpath << ", " << stmtPost << ") # Death of good resp ptr causes a mem leak\n"; /* pri_cat=mem_leak */
                                        if (isa<AllocaInst>(trace_thru_GEP(killedVar)) && seenCPaths.count(std::string("*") + cpath)) {
                                            outFile << "!responsible(*" << cpath << ") | !good(*" << cpath << ", " << stmtPost << ") # Death of good resp ptr causes a mem leak\n"; /* pri_cat=mem_leak */
                                        }
                                    }
                                }
                            }
                            check_on_waiting_to_die();
                        }
                        if (isa<ReturnInst>(&I) && !DisableMemLeak) {
                            for (int i = 0; i < curFunc->arg_size(); i++) {
                                Argument* arg = curFunc->getArg(i);
                                std::string argName = getArgName(arg);
                                if (!isDryRun) {
                                    std::string stmtPost = getStatementLabel(I, "post");
                                    assert(stmtPost == std::string("end"));
                                    outFile << "!responsible(" << argName << ") | !good(" << argName << ", " << stmtPost << ") # Death of good resp ptr causes a mem leak\n"; /* pri_cat=mem_leak */
                                }
                            }
                        }
                        if (I.getType()->isPointerTy() && has_nondummy_uses(&I)) {
                            currentLiveVars.insert(&I);
                        }
                    }
                    if (isa<ReturnInst>(&I) && !DisableBorrowChecks) {
                        if (!isDryRun) {
                            bool gotOne = false;
                            for (const Borrow& borrow : curBI.borrows_by_src) {
                                bool has_deref = cpath_has_deref(borrow.from);
                                if (has_deref) {
                                    continue;
                                }
                                std::string src_name = cpath_part_id_to_str[borrow.from[0]];
                                Value* srcLLVM = cpathToValue[src_name];
                                //errs() << "src_name: " << src_name;
                                //if (!srcLLVM) {
                                //    errs() << " is null.\n";
                                //} else if (!isa<AllocaInst>(srcLLVM)) {
                                //    errs() << " is not an alloca.\n";
                                //} else {
                                //    errs() << "\n";
                                //}
                                if (srcLLVM && isa<AllocaInst>(srcLLVM)) {
                                    if (!gotOne) {
                                        outFile << "# Function exit point\n";
                                        gotOne = true;
                                    }
                                    outFile << "false # Borrow of local var (" << src_name << ") escapes function.  Borrowed by " << cpath_to_str(borrow.to) << "\n";
                                }
                            }
                            //errs() << "\nFunction " << curFunc->getName().str() << ": borrows at exit:\n";
                            //errs() << dump_borrows_to_str(curBI) << "\n";
                            static std::regex pattern(R"(([A-Za-z0-9_:]+):@orig([A-Za-z0-9_:!+<>]+))");
                            std::vector<Borrow> end_borrows;
                            for (const Borrow& borrow : curBI.borrows_by_dest) {
                                std::smatch matches;
                                std::string arg;
                                int deref_count = -1;
                                std::string src_base = cpath_part_id_to_str[borrow.from[0]];
                                Borrow new_borrow;
                                if (std::regex_match(src_base, matches, pattern)) {
                                    std::string src_str = cpath_to_str(borrow.from, false);
                                    src_str = replace_substr(src_str, ":@orig", "");
                                    src_str = rename_transformed_cpath_back(src_str);
                                    CPath src_cpath = parse_cpath(src_str);
                                    new_borrow = (Borrow){.to = borrow.to, .from = src_cpath};
                                } else if (argNameSet.count(src_base)) {
                                    new_borrow = (Borrow){.to = borrow.to, .from = borrow.from};
                                }
                                bool too_long = false;
                                for (CPath new_cpath : {new_borrow.to, new_borrow.from}) {
                                    static bool already_warned = false;
                                    if (new_cpath.size() > MAX_CPATH_DEPTH) {
                                        too_long = true;
                                        if (!already_warned) {
                                            already_warned = true;
                                            errs() << "Warning: MAX_CPATH_DEPTH exceeded when computing end borrows; ignoring such C-paths.\n";
                                            errs() << "Info: one such path: " << cpath_to_str(new_cpath) << "\n";
                                        }
                                    }
                                }
                                if (!too_long) {
                                    end_borrows.push_back(new_borrow);
                                }
                            }
                            //errs() << "\nFunction " << curFunc->getName().str() << ": borrows at exit:\n";
                            std::string func_name = curFunc->getName().str();
                            auto& pomBorrowsForCurFunc = pomBorrows[func_name];
                            std::string func_name_prefix = func_name + "::";
                            for (const Borrow& borrow : end_borrows) {
                                if (borrow.from == borrow.to) {continue;}
                                CPath src = borrow.from;
                                std::string src_base_var = get_arg_name_of_base_var(cpath_part_id_to_str[src[0]]);
                                src[0] = get_cpath_part_id_from_str(src_base_var);
                                //errs() << "  src = " << cpath_to_str(src) << "\n";
                                std::string dest_name = replace_substr(cpath_to_str(borrow.to, false), ":arg:", "::args::");
                                //for (std::string listed_src : pomBorrowsForCurFunc[cpath_to_str(borrow.to)]) {
                                //    errs() << "  listed source: " << listed_src << "\n";
                                //}
                                if (!has_prefix(dest_name, func_name_prefix)) {
                                    //errs() << "  Skipping dest " << dest_name << "\n";
                                    continue;
                                }
                                if (dest_name.find(":@orig") != std::string::npos) {
                                    //errs() << "  Skipping dest " << dest_name << "\n";
                                    continue;
                                }
                                dest_name = cpath_to_str(parse_cpath(dest_name));
                                outFile << "borrows_from(" << dest_name << ", " << cpath_to_str(src) << ")\n";
                                if (pomBorrowsForCurFunc[dest_name].count(cpath_to_str(src))) {continue;}
                                //errs() << "Old pomBorrows[" << func_name << "][" << dest_name << "] = [";
                                //for (std::string existing_src : pomBorrowsForCurFunc[dest_name]) {
                                //    errs() << " " << existing_src;
                                //}
                                //errs() << " ]; adding " << cpath_to_str(src) << "\n";
                                pomBorrowsForCurFunc[dest_name].insert(cpath_to_str(src));
                                hasUpdatedBorrows = true;
                                if (!gotOne) {
                                    outFile << "# Function exit point\n";
                                    gotOne = true;
                                }
                                if (pomFuncs.count(func_name)) {
                                    pomMissingErrors.push_back("Func " + func_name + ": missing lifetime: " + dest_name + " borrows from " + cpath_to_str(src));
                                } else if (!WholeProg) {
                                    errs() << "WARNING: Func " << func_name << ": missing lifetime: " << dest_name << " borrows from " << cpath_to_str(src) << "\n";
                                }
                            }
                        }
                    }
                    prevInst = &I;
                }
                if (!isDryRun) {
                    outFile << "</instruction>\n";
                }

                // Handle inter-basic-block control flow constraints
                if (!isDryRun) {
                    outFile << "# Flow to successor basic block(s)\n";
                }
                processControlFlow(BB);
                for (CPath cpath : postponed_kills) {
                    handleCPathBecomingDead(cpath);
                }
                check_on_waiting_to_die();
                if (!DisableBorrowChecks) {
                    if (borrows_at_end_of_bb[&BB] != curBI) {
                        borrows_at_end_of_bb[&BB] = curBI; // this does a deep copy
                        changed_borrows = true;
                    }
                }
                
                if (!isDryRun) {
                    outFile << "</BasicBlock>\n";
                }
            }
        }
        // Contraints to enforce that all responsible pointers are mutable
        {
            outFile << "# All responsible pointers are mutable\n";
            for (const std::string &cpath : seenCPaths) {
                if (cpath == std::string("NULL_CONST")) {continue;}
                // responsible(x) -> mut(x)
                outFile << "!responsible(" << cpath << ") | mut(" << cpath << ")\n";
            }
        }
        for (GlobalValue* gv : seen_globals) {
            outFile << "!responsible(" << getCPath(*gv) << ") # The address of a global is never responsible.\n";
        }
        
        outFile << "</function>\n\n";
        if (!NumberedIR.empty()) {
            numirFile << "}\n\n";
        }
        
        reset_at_end_of_func();
        bool modified = true;
        return modified;
    }

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

    int getArgTypePtrDepth(llvm::Function *F, const std::string &argName) {
        if (!F) return -1;

        // Get DI subprogram/type info
        DISubprogram *SP = F->getSubprogram();
        if (!SP) return -1;
        auto *ST = SP->getType();
        if (!ST) return -1;

        DITypeRefArray Types = ST->getTypeArray();
        if (Types.size() == 0) return -1;

        // Find the IR-argument index whose name matches argName
        int argIdx = getArgIdxFromName(F, argName);
        if (argIdx < 0) return -1;

        // In DISubroutineType, index 0 = return type; params start at 1
        size_t diIdx = static_cast<size_t>(argIdx + 1);
        if (diIdx >= Types.size()) return -1;

        DIType *ArgTy = Types[diIdx];
        if (!ArgTy) return -1;

        return countPointerIndirectionFromDI(ArgTy);
    }

    int getReturnTypePtrDepth(Function *F) {
        // Get the debug info subprogram for this function
        DISubprogram *SP = F->getSubprogram();
        if (!SP) {return -1;}
        
        // Get the subroutine type
        DISubroutineType *ST = SP->getType();
        if (!ST) {return -1;}
        
        // Get the type array - first element is return type, rest are parameters
        DITypeRefArray Types = ST->getTypeArray();
        if (Types.size() == 0) {return -1;}
        
        // First element is the return type
        DIType *ReturnType = Types[0];
        if (!ReturnType) {return -1;}
        
        return countPointerIndirectionFromDI(ReturnType);
    }
    
    void buildDebugInfoMapping(Function &F) {
        llvm::BasicBlock* entryBlock = &F.getEntryBlock();
        // Look for debug declare and value intrinsics to map LLVM values to source names
        for (BasicBlock &BB : F) {
            bool isEntryBB = (&BB == entryBlock);
            std::set<Instruction*> instr_set;
            for (Instruction &I : BB) {
                instr_set.insert(&I);
            }
            for (Instruction &I : BB) {
                if (DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(&I)) {
                    if (Value *V = DDI->getAddress()) {
                        if (DILocalVariable *DIVar = DDI->getVariable()) {
                            valueToSourceName[V] = DIVar->getName().str();
                        }
                    }
                } else if (DbgValueInst *DVI = dyn_cast<DbgValueInst>(&I)) {
                    if (Value *V = DVI->getValue()) {
                        if (DILocalVariable *DIVar = DVI->getVariable()) {
                            Instruction* VI = dyn_cast<Instruction>(V);
                            bool alreadyNamed = (valueToSourceName.find(V) != valueToSourceName.end());
                            if ((!alreadyNamed) || (instr_set.find(VI) != instr_set.end()) || (isa<Argument>(*V) && isEntryBB)) {
                                valueToSourceName[V] = DIVar->getName().str();
                                // V->dump();
                                // outs() << valueToSourceName[V] << "\n\n";
                            }
                        }
                    }
                }
            }
        }
    }
    
    void processInstruction(Instruction &I) {

        static std::unordered_set<unsigned> warned_opcodes;
        
        this->curInstruction = &I;
        this->curInstructionID = getInstructionId(I);

        if (TargetDebugInstId == getInstructionId(I) && !isDryRun) {
            errs() << "\nInstruction " << getInstructionId(I) << ":\n";
            errs() << dump_borrows_to_str(curBI);
            errs() << "\n";
        }
        for (const llvm::Use &U : I.operands()) {
            if (GlobalValue* g = dyn_cast<GlobalValue>(U.get())) {
                seen_globals.insert(g);
            }
        }
        if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
            processAlloca(*AI);
        } else if (CallInst *CI = dyn_cast<CallInst>(&I)) {
            if (Function *F = getCalledFunc(CI)) {
                if (F->getName() == "malloc" || F->getName() == "calloc") {
                    //if (!isDryRun) {outFile << "# (call to " << F->getName().str() << ")\n";}
                    processMalloc(*CI);
                } else if (F->getName() == "free" || F->getName() == "fclose") {
                    //if (!isDryRun) {outFile << "# (call to " << F->getName().str() << ")\n";}
                    processFree(*CI);
                } else if (F->getName() == "__pom_var_store") {
                    processPomVarStore(*CI);
                } else {
                    // Generic function call
                    processDirectCall(*CI);
                }
            } else {
                // Indirect function call
                if (!NoWarnFuncPtr) {
                    NoWarnFuncPtr = true; // Only warn once.
                    errs() << "Warning: Unresolved indirect function call (first occurrence: " << getStatementLabel(I, "") << ").\n";
                    seen_unsupported.insert("unresolved_func_ptr");
                }
                std::string stmtPost = getStatementLabel(*CI, "post");
                std::string stmtPre = getStatementLabel(*CI, "pre");
                generatePreservationConstraints(stmtPre, stmtPost, {}, {});
            }
        } else if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
            processLoad(*LI);
        } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
            processStore(*SI);
        } else if (PHINode *PN = dyn_cast<PHINode>(&I)) {
            processPhi(*PN);
        } else if (ReturnInst *RI = dyn_cast<ReturnInst>(&I)) {
            processReturn(*RI);
        } else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(&I)) {
            processGEP(*GEP);
        } else if (SelectInst *SI = dyn_cast<SelectInst>(&I)) {
            processSelect(*SI);
        } else if (dyn_cast<FreezeInst>(&I)) {
            processFreeze(I);
        } else {
            unsigned opcode = I.getOpcode();
            bool recognized = (opcode == Instruction::ICmp || opcode == Instruction::PtrToInt);
            if (!recognized) {
                bool hasPtrOperands = false;
                for (const llvm::Use &U : I.operands()) {
                    if (U->getType()->isPointerTy()) {
                        hasPtrOperands = true;
                        break;
                    }
                }
                if (hasPtrOperands) {
                    if (!warned_opcodes.count(opcode)) {
                        warned_opcodes.insert(opcode);
                        errs() << "Warning: unrecognized opcode '" << I.getOpcodeName() << "' with a pointer-valued operand\n";
                    }
                }
            }
            // Other instructions - generate preservation constraints
            std::string stmtPost = getStatementLabel(I, "post");
            std::string stmtPre = getStatementLabel(I, "pre");
            generatePreservationConstraints(stmtPre, stmtPost, {}, {});
        }
    }

    void processAlloca(AllocaInst &alloca) {
        std::string ptrName = getCPath(alloca);
        std::string stmtPost = getStatementLabel(alloca, "post");
        std::string stmtPre = getStatementLabel(alloca, "pre");
        
        if (!isDryRun) {
            outFile << "!responsible(" << ptrName << ")\n";
            outFile << "good(" << ptrName << ", " << stmtPost << ")\n";
        }
        
        add_zombie_to_uninit_pointee(ptrName, stmtPost, &alloca);

        // Add to seen C-paths
        add_seen_cpath_str(ptrName);
        
        // good(x, S-post) <-> good(x, S-pre) for all other x
        generatePreservationConstraints(stmtPre, stmtPost, {ptrName}, {});
    }
    
    void processMalloc(CallInst &CI) {
        std::string ptrName = getCPath(CI);
        std::string stmtPost = getStatementLabel(CI, "post");
        std::string stmtPre = getStatementLabel(CI, "pre");
        
        // responsible(p), good(p, S-post), null(p, S-post)
        if (!isDryRun) {
            outFile << "responsible(" << ptrName << ")\n";
            outFile << "good(" << ptrName << ", " << stmtPost << ") # Good ptr may be returned by malloc\n";
            outFile << "null(" << ptrName << ", " << stmtPost << ") # Null ptr may be returned by malloc\n";
            outFile << "!zombie(" << ptrName << ", " << stmtPost << ") # Zombie is never returned by malloc\n";
        }
        
        Function *F = CI.getCalledFunction();
        if (F->getName() != "calloc") {
            add_zombie_to_uninit_pointee(ptrName, stmtPost, &CI);
        }
        
        // Add to seen C-paths
        add_seen_cpath_str(ptrName);
        
        // Preservation constraints for other variables
        generatePreservationConstraints(stmtPre, stmtPost, {ptrName}, {});
    }

    void add_zombie_to_uninit_pointee(std::string& ptr, std::string& stmtPost, Instruction* inst) {
        // ptr itself is initialized, but *ptr isn't.
        std::string starPfx = "*";
        int stopDepth = getDeepPtrCopyStopDepth(ptr, ptr);
        bool haveRealStopDepth = (stopDepth != MAX_PTR_DEPTH);
        while (true) {
            if (haveRealStopDepth || seenCPaths.count(starPfx + ptr)) {
                if (starPfx.length() > stopDepth) {
                    if (stopDepth == MAX_PTR_DEPTH) {
                        errs() << "WARNING: pointer depth is too large, not going any further (inst=" << getInstructionId(*inst) << ", ptr=" << ptr << ")\n";
                    }
                    break;
                }
                add_seen_cpath_str(starPfx + ptr);
                if (!isDryRun) {
                    outFile << "!responsible(" << starPfx << ptr << ") | zombie(" << starPfx << ptr << ", " << stmtPost << ") # Uninit values are considered zombies\n"; /* pri_cat=uninit */
                }
                starPfx += "*";
            } else {
                break;
            }
        }
    }

    Value* trace_thru_freeze(Value* val) {
        if (isa<FreezeInst>(val)) {
            return dyn_cast<FreezeInst>(val)->getOperand(0);
        } else {
            return val;
        }
    }
    
    void processFree(CallInst &CI) {
        Value* arg = CI.getArgOperand(0);
        std::string ptrName = getCPath(*arg);
        std::string stmtPost = getStatementLabel(CI, "post");
        std::string stmtPre = getStatementLabel(CI, "pre");
        
        std::string destructor_name = getCalledFunc(&CI)->getName().str();
        
        if (!isDryRun) {
            outFile << "responsible(" << ptrName << ")\n";
            outFile << "!zombie(" << ptrName << ", " << stmtPre << ") # A zombie ptr should not be freed.\n"; /* pri_cat=double_free */
            if (destructor_name == std::string("fclose")) {
                outFile << "!null(" << ptrName << ", " << stmtPre << ") # A null ptr should not be given to fclose().\n"; /* pri_cat=invalid_free */
            }
            outFile << "!good(" << ptrName << ", " << stmtPre << ") | zombie(" << ptrName << ", " << stmtPost << ") # A good ptr becomes a zombie after getting freed.\n"; /* pri_cat=basic */
            outFile << "!null(" << ptrName << ", " << stmtPre << ") | null(" << ptrName << ", " << stmtPost << ") # Preservation \n";
            if (!DisableMemLeak) {
                outFile << "!responsible(*" << ptrName << ") | !good(*" << ptrName << ", " << stmtPre << ") # Freeing a memory block holding a good responsible pointer causes a memory leak\n"; /* pri_cat=mem_leak */
            }
            if (isa<UndefValue>(trace_thru_freeze(arg))) {
                outFile << "false # An uninitialized pointer must not be passed to free.\n";
            }
            if (isa<PoisonValue>(trace_thru_freeze(arg))) {
                outFile << "false # A poison value must not be passed to free.\n";
            }
        }
        
        if (!DisableBorrowChecks && !isDryRun) {
            std::vector<Borrow> borrows;
            getBorrowsBySrcRootedAt(parse_cpath(ptrName), &borrows);
            for (Borrow borrow : borrows) {
                if (cpath_has_deref(borrow.from)) {
                    continue;
                }
                outFile << "false # Cannot free a ptr (" << ptrName << ") that is borrowed.  Borrowed by:";
                for (Borrow& borrow : borrows) {
                    if (cpath_has_deref(borrow.from)) {
                        continue;
                    }
                    CPath borrower = borrow.to;
                    std::string borrower_name = cpath_to_str(borrower);
                    outFile << " ";
                    outFile << borrower_name;
                    Value* otherValue = cpathToValue[borrower_name];
                    if (otherValue) {
                        if (Instruction* otherInst = dyn_cast<Instruction>(otherValue)) {
                            outFile << std::string(" (I") << getInstructionId(*otherInst) << ")";
                        }
                    }
                }
                outFile << "\n";
                break;
            }
        }

        // Add to seen C-paths
        add_seen_cpath_str(ptrName);
        
        // Preservation constraints for other variables
        generatePreservationConstraints(stmtPre, stmtPost, {ptrName}, {});
    }

    int getArgIdxFromName(Function* F, std::string wantedArgName) {
        std::vector<std::string> argNames;
        argNames.resize(F->arg_size());
        for (int idx = 0; idx < F->arg_size(); idx++) {
            Argument* formal_param_obj = F->getArg(idx);
            std::string curArgName;
            if (formal_param_obj->hasName()) {
                curArgName = formal_param_obj->getName().str();
            } else {
                std::vector<std::string>& argNames = funcArgName[F->getName().str()];
                if (idx < argNames.size()) {
                    curArgName = argNames[idx];
                }
            }
            if (curArgName == wantedArgName) {
                return idx;
            }
        }
        return -1;
    }

    void processDirectCall(CallInst &CI) {
        std::string stmtPost = getStatementLabel(CI, "post");
        std::string stmtPre = getStatementLabel(CI, "pre");
        Function *F = getCalledFunc(&CI);
        std::string func_name = F->getName().str();
        std::set<std::string> changedCPaths = {};
        static std::set<std::string> warnedFuncs = {};

        if (func_name == "__dump_borrows" && !isDryRun) {
            int line_num;
            if (const DebugLoc &DL = CI.getDebugLoc()) {
                line_num = DL.getLine();
            }
            errs() << "Borrows at line " << line_num << ":\n";
            errs() << dump_borrows_to_str(curBI);
        }

        std::vector<std::string> formal_param_cpath_vec;
        std::vector<std::string> formal_param_name_vec;
        std::vector<std::string> actual_arg_name_vec;
        formal_param_cpath_vec.reserve(F->arg_size());
        formal_param_name_vec.reserve(F->arg_size());
        actual_arg_name_vec.reserve(F->arg_size());
        std::map<std::string, std::string> formal_to_actual;

        // Process each argument
        for (int i = 0; i < CI.arg_size(); i++) {
            if (!CI.getArgOperand(i)->getType()->isPointerTy()) {
                continue;
            }
            std::string formal_param_name;
            if (i < F->arg_size()) {
                Argument* formal_param_obj = F->getArg(i);
                if (formal_param_obj->hasName()) {
                    formal_param_name = formal_param_obj->getName().str();
                } else {
                    std::vector<std::string>& argNames = funcArgName[func_name];
                    if (i < argNames.size()) {
                        formal_param_name = argNames[i];
                    }
                    if (formal_param_name.empty()) {
                        if (!NoWarnArgNames) {
                            if (warnedFuncs.count(func_name) == 0) {
                                errs() << "Warning: an argument of function " << func_name << " is missing a name.\n";
                                warnedFuncs.insert(func_name);
                            }
                        }
                        formal_param_name = std::string("arg-") + std::to_string(i);
                    }
                }
            } else {
                formal_param_name = "var-args";
            }
            
            std::string formal_param = func_name + "::args::" + formal_param_name;
            formal_param_cpath_vec.push_back(formal_param);
            formal_param_name_vec.push_back(formal_param_name);

            std::string act_arg = getCPath(*CI.getArgOperand(i));
            actual_arg_name_vec.push_back(act_arg);

            formal_to_actual[formal_param_name] = act_arg;
            
            add_seen_cpath_str(act_arg);
            changedCPaths.insert(act_arg);
            
            if (!isDryRun) {
                outFile << "!zombie(" << act_arg << ", " << stmtPre << ") # Don't pass zombies to functions.\n"; /* pri_cat=zombie_arg */
            }
            
            // Use PtrCopyConstraints for arguments
            PtrCopyArgs ptrCopyArgs = {
                .dest = formal_param,
                .src = act_arg,
                .stmtPre = stmtPre,
                .stmtPost = stmtPost,
                .stmtPostDest = "start",
                .inst = &CI,
            };
            PtrCopyConstraints(ptrCopyArgs);
            
            // Function summary constraints for pointed-to objects
            std::string starPfx = "*";
            int stopDepth = getDeepPtrCopyStopDepth(formal_param, act_arg);
            bool haveRealStopDepth = (stopDepth != MAX_PTR_DEPTH);
            while (true) {
                if (haveRealStopDepth || seenCPaths.count(starPfx + act_arg)) {
                    if (starPfx.length() > stopDepth) {
                        if (stopDepth == MAX_PTR_DEPTH) {
                            errs() << "WARNING: pointer depth is too large, not going any further!\n";
                        }
                        break;
                    }
                    
                    add_seen_cpath_str(starPfx + act_arg);

                    // good(*userfunc::formal_param_i, end) -> good(*act_arg_i, S-post) and ditto for null and zombie
                    if (!isDryRun) {
                        outFile << "!good(" << starPfx << formal_param << ", end) | good(" << starPfx << act_arg << ", " << stmtPost << ") # Callsite: DeepPtrCopy\n";
                        outFile << "!null(" << starPfx << formal_param << ", end) | null(" << starPfx << act_arg << ", " << stmtPost << ") # Callsite: DeepPtrCopy\n";
                        outFile << "!zombie(" << starPfx << formal_param << ", end) | zombie(" << starPfx << act_arg << ", " << stmtPost << ") # Callsite: DeepPtrCopy\n";
                    }

                    starPfx += "*";
                } else {
                    break;
                }
            }
        }

        if (func_name == "realloc") {
            std::string& arg0 = actual_arg_name_vec[0];
            argsToRealloc.insert(arg0);
            if (!isDryRun) {
                outFile << "!good(" << arg0 << ", " << stmtPre << ") | good(" << arg0 << ", " << stmtPost << ") # If realloc fails, the passed-in resp ptr stays good.\n";
            }
        }

        if (!DisableBorrowChecks) {
            for (std::string formal_param : formal_param_cpath_vec) {
                handleCPathBecomingDead(parse_cpath(formal_param));
            }
            check_on_waiting_to_die();

            //For every CPath src_formal rooted at "$callee::return" or "$callee::args:$arg_name":
            //    For every CPath dest_formal rooted at "$callee::return" or "$callee::args:$arg_name":
            //        Let src_actual and dest_actual be the corresponding CPaths using the actual arguments or actual return value.
            //        If the lifetime for dest_formal in POM YML model includes src_formal, then add Borrow(to=dest_actual, from=src_actual).


            auto& pomBorrowsForCallee = pomBorrows[func_name];
            std::vector<Borrow> borrowsFromSummary;
            
            //for (auto const& [dest_formal_cpath_str, src_list] : pomBorrowsForCallee) {
            //    errs() << "pomBorrows[" << func_name << "][" << dest_formal_cpath_str << "]:";
            //    for (const std::string& src_formal_cpath_str : src_list) {
            //        errs() << " " << src_formal_cpath_str;
            //    }
            //    errs() << "\n";
            //}

            std::set<CPathPart> arg_set;
            //errs() << "Arg set: [";
            for (std::string act_arg_name : actual_arg_name_vec) {
            //    errs() << " " << act_arg_name;
                CPath arg_cpath = parse_cpath(act_arg_name);
                assert(arg_cpath.size() == 1);
                arg_set.insert(arg_cpath[0]);
            }
            //errs() << "]\n";

            //if (true) {
            //    errs() << "Borrows before renaming prior to call to " << func_name << " at instruction I" << curInstructionID << ":\n";
            //    errs() << dump_borrows_to_str(curBI);
            //}

            std::map<CPath, CPath> rename_map;
            std::string rename_prefix = "pre-I" + std::to_string(curInstructionID) + ":";
            rename_borrows_before_func_call(arg_set, &rename_map, rename_prefix);

            //errs() << "rename_map: [\n";
            //for (auto const& [old_cpath, new_cpath] : rename_map) {
            //    errs() << " " << cpath_to_str(old_cpath) << " ==> " << cpath_to_str(new_cpath) << "\n";
            //}
            //errs() << "]\n";

            for (auto const& [normal_cpath, transmogrified_cpath] : rename_map) {
                curBI.add((Borrow){.to = normal_cpath, .from = transmogrified_cpath});
            }
            //if (true) {
            //    errs() << "Borrows after renaming prior to call to " << func_name << " at instruction I" << curInstructionID << ":\n";
            //    errs() << dump_borrows_to_str(curBI);
            //}

            bool returnHasUses = has_nondummy_uses(&CI);
            CPathPart actual_return_cpath_base;
            if (!CI.getType()->isVoidTy()) {
                std::string actual_return_str = getCPath(CI);
                actual_return_cpath_base = parse_cpath(actual_return_str)[0];
                formal_to_actual["return"] = actual_return_str;
            }

            for (auto const& [dest_formal_cpath_str, src_list] : pomBorrowsForCallee) {
                CPath dest_cpath = parse_cpath(dest_formal_cpath_str);
                std::string dest_formal_name = get_arg_name_of_base_var(cpath_part_id_to_str[dest_cpath[0]]);
                std::string dest_actual_name = formal_to_actual[dest_formal_name];
                if (dest_actual_name.empty()) {
                    errs() << "Error: Unrecognized argument name \"" << dest_formal_cpath_str << "\" (func: " << func_name << ")\n";
                    continue;
                }
                dest_cpath[0] = get_cpath_part_id_from_str(dest_actual_name);
                for (const std::string& src_formal_cpath_str : src_list) {
                    CPath src_cpath = parse_cpath(src_formal_cpath_str);
                    std::string src_formal_name = cpath_part_id_to_str[src_cpath[0]];
                    std::string src_actual_name = formal_to_actual[src_formal_name];
                    if (src_actual_name.empty()) {
                        errs() << "Error: Unrecognized argument name \"" << src_formal_cpath_str << "\" (func: " << func_name << ")\n";
                        continue;
                    }
                    src_cpath[0] = get_cpath_part_id_from_str(src_actual_name);
                    src_cpath = rename_cpath_before_func_call(src_cpath, arg_set, &rename_map, rename_prefix);
                    borrowsFromSummary.push_back((Borrow){.to = dest_cpath, .from = src_cpath});
                }
            }
            //if (borrowsFromSummary.size() > 0 && !isDryRun) {
            //    errs() << "Borrows from call to " << func_name << " at instruction I" << curInstructionID << " (step 1):\n";
            //    errs() << dump_borrows_to_str(curBI);
            //    errs() << "\n";
            //}
            for (Borrow& borrow : borrowsFromSummary) {
                CPath src_cpath = borrow.from;
                CPath dest_cpath = borrow.to;
                if (src_cpath == dest_cpath) {
                    continue;
                }
                if (!returnHasUses && dest_cpath[0] == actual_return_cpath_base) {
                    continue;
                }
                //if (!isDryRun) {
                //    errs() << "  Adding borrow from " << cpath_to_str(src_cpath) << " to " << cpath_to_str(dest_cpath) << "\n";
                //}
                bool makes_cycle = false;
                add_borrow(borrow, &makes_cycle);
                if (makes_cycle) {
                    errs() << "(during instruction " << curInstructionID << ")\n\n";
                }
            }
            //if (borrowsFromSummary.size() > 0 && !isDryRun) {
            //    errs() << "Borrows from call to " << func_name << " at instruction I" << curInstructionID << " (step 2):\n";
            //    errs() << dump_borrows_to_str(curBI);
            //    errs() << "\n";
            //}
            for (auto const& [normal_cpath, transmogrified_cpath] : rename_map) {
                handleCPathBecomingDead(transmogrified_cpath, true);
            }
            check_on_waiting_to_die();
        }
         
        
        
        // Handle return value if the function returns a pointer
        if (CI.getType()->isPointerTy()) {
            std::string tName = getCPath(CI);
            std::string returnName = func_name + "::return";
            
            add_seen_cpath_str(tName);
            changedCPaths.insert(tName);

            if (!isDryRun) {
                // responsible(userfunc::return) <-> responsible(t)
                outFile << "!responsible(" << returnName << ") | responsible(" << tName << ")\n";
                outFile << "!responsible(" << tName << ") | responsible(" << returnName << ")\n";

                auto itDepMut = pomDepMutRet.find(func_name);
                if (itDepMut != pomDepMutRet.end()) {
                    std::string rhs_arg_formal = itDepMut->second;
                    std::string rhs_arg_actual = formal_to_actual[rhs_arg_formal];
                    if (rhs_arg_actual.empty()) {
                        errs() << "Error: Unrecognized argument name \"" << rhs_arg_formal << "\" (func: " << func_name << ")\n";
                    } else {
                        outFile << "!mut(" << tName << ") | mut(" << rhs_arg_actual << ")\n";
                    }
                } else {
                    // mut(t) -> mut(userfunc::return)
                    outFile << "!mut(" << tName << ") | mut(" << returnName << ")\n";
                }

                // good(userfunc::return, end) -> good(t, S-post) and ditto for null and zombie
                outFile << "!good(" << returnName << ", end) | good(" << tName << ", " << stmtPost << ")\n";
                outFile << "!null(" << returnName << ", end) | null(" << tName << ", " << stmtPost << ")\n";
                outFile << "!zombie(" << returnName << ", end) | zombie(" << tName << ", " << stmtPost << ")\n";
            }

            
            // Use DeepPtrCopyConstraints for return value
            PtrCopyArgs returnCopyArgs = {
                .dest = tName,
                .src = returnName,
                .stmtPre = "end",
                .stmtPost = stmtPost,
                .stmtPostDest = stmtPost,
                .inst = &CI
            };
            std::vector<CPath> dest_loc_aliases = {parse_cpath(returnCopyArgs.dest)};
            DeepPtrCopyConstraints(returnCopyArgs, dest_loc_aliases);
        }

        // Generate preservation constraints for variables not in changedCPaths
        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, {});
    }

    void getAliasesOf(CPath ptr_cpath, bool only_must, std::vector<CPath>* aliases) {
        // The output list includes the original pointer itself.
        // We intentionally consider only straight-up and straight-down alias chains.
        // For mutable borrows, this finds all the aliases.  For non-mutable borrows,
        // we can soundly ignore other aliases (at a cost of some false positives).
        aliases->push_back(ptr_cpath);
        std::set<CPath> already_seen;
        already_seen.insert(ptr_cpath);
        static bool already_warned_max_cpath = false;

        for (int dir : {0, 1}) {
            // dir==0: aliases that borrow from ptr_cpath (directly or indirectly)
            // dir==1: aliases from which ptr_cpath borrows (directly or indirectly)
            Worklist<CPath> worklist;
            worklist.push(ptr_cpath);
            //if (!isDryRun) {
            //    errs() << "\nFinding aliases of " << cpath_to_str(ptr_cpath) << ", dir=" << dir << "\n";
            //}
            while (!worklist.empty()) {
                CPath full_cpath = worklist.pop();

                //if (!isDryRun) {
                //    errs() << "\n  full_cpath: " << cpath_to_str(full_cpath) << "\n";
                //}
                
                // If p1 is an alias of p2, then *p1 is an alias of *p2.
                // If p1 is an alias of p2, then p1.fld is an alias of p2.fld.
                for (int cpath_split_pt = 0; cpath_split_pt < full_cpath.size(); cpath_split_pt++) {
                    CPath cpath_prefix(full_cpath.begin(), full_cpath.begin() + cpath_split_pt + 1);
                    CPath cpath_suffix(full_cpath.begin() + cpath_split_pt + 1, full_cpath.end());
                    //if (!isDryRun) {
                    //    errs() << "  Prefix: " << cpath_to_str(cpath_prefix) << "\n";
                    //    errs() << "  Suffix: " << cpath_to_str(cpath_suffix, false) << "\n";
                    //}

                    // Process a list of borrows that starts with itLo and ends right before itHi.
                    auto process_borrows = [&](auto itLo, auto itHi) {
                        for (auto it = itLo; it != itHi; ++it) {
                            const Borrow& curBorrow = *it;
                            bool is_must = curBI.is_must_borrow[curBorrow];
                            if (only_must && !is_must) {
                                continue;
                            }
                            assert((dir == 0) ? curBorrow.from == cpath_prefix : curBorrow.to == cpath_prefix);
                            CPath ali = (dir == 0) ? curBorrow.to : curBorrow.from;
                            ali.reserve(ali.size() + cpath_suffix.size());
                            for (CPathPart part : cpath_suffix) {
                                ali.push_back(part);
                            }
                            if (ali.size() > MAX_CPATH_DEPTH) {
                                if (!already_warned_max_cpath) {
                                    already_warned_max_cpath = true;
                                    errs() << "Warning: MAX_CPATH_DEPTH exceeded when computing aliases; ignoring such C-paths.\n";
                                    errs() << "Info: one such path: " << cpath_to_str(ali) << "\n";
                                }
                                continue;
                            }
                            if (already_seen.count(ali) == 0) {
                                aliases->push_back(ali);
                                worklist.push(ali);
                                already_seen.insert(ali);
                            }
                        }
                    };

                    CPath cur_cpath_pfx_end = cpath_prefix;
                    cur_cpath_pfx_end.push_back(0);

                    if (dir == 0) {
                        // Iterate over all borrows where from=cpath_prefix
                        const Borrow lower = {.to = {0}, .from = cpath_prefix};
                        const Borrow upper = {.to = {0}, .from = cur_cpath_pfx_end};
                        process_borrows(curBI.borrows_by_src.lower_bound(lower),
                                        curBI.borrows_by_src.upper_bound(upper));
                    } else {
                        // Iterate over all borrows where to=cpath_prefix
                        const Borrow lower = {.to = cpath_prefix, .from = {0}};
                        const Borrow upper = {.to = cur_cpath_pfx_end, .from = {0}};
                        process_borrows(curBI.borrows_by_dest.lower_bound(lower),
                                        curBI.borrows_by_dest.upper_bound(upper));
                    }
                }
            }
        }
    }

    bool hasBorrowsFrom(CPath ptr_cpath) {
        CPath ptr_cpath_plus_max = ptr_cpath;
        ptr_cpath_plus_max.push_back(CPathPartMax);
        const Borrow lower = {.to = {0}, .from = ptr_cpath};
        const Borrow upper = {.to = {0}, .from = ptr_cpath_plus_max};
        auto itLo = curBI.borrows_by_src.lower_bound(lower);
        auto itHi = curBI.borrows_by_src.lower_bound(upper);
        return (itLo != itHi);
    }

    void getBorrowsBySrcRootedAt(CPath ptr_cpath, std::vector<Borrow>* borrows) {
        CPath ptr_cpath_plus_max = ptr_cpath;
        ptr_cpath_plus_max.push_back(CPathPartMax);
        const Borrow lower = {.to = {0}, .from = ptr_cpath};
        const Borrow upper = {.to = {0}, .from = ptr_cpath_plus_max};
        auto itLo = curBI.borrows_by_src.lower_bound(lower);
        auto itHi = curBI.borrows_by_src.lower_bound(upper);
        for (auto it = itLo; it != itHi; ++it) {
            borrows->push_back(*it);
        }
    }

    void getBorrowsByDestRootedAt(CPath ptr_cpath, std::vector<Borrow>* borrows) {
        CPath ptr_cpath_plus_max = ptr_cpath;
        ptr_cpath_plus_max.push_back(CPathPartMax);
        const Borrow lower = {.to = {0}, .from = ptr_cpath};
        const Borrow upper = {.to = {0}, .from = ptr_cpath_plus_max};
        auto itLo = curBI.borrows_by_dest.lower_bound(lower);
        auto itHi = curBI.borrows_by_dest.lower_bound(upper);
        for (auto it = itLo; it != itHi; ++it) {
            borrows->push_back(*it);
        }
    }

    void get_borrow_cpaths_rooted_at(CPath base_var, std::set<CPath>* cpath_set) {
        std::vector<Borrow> borrows;
        getBorrowsBySrcRootedAt(base_var, &borrows);
        for (const Borrow& borrow : borrows) {
            cpath_set->insert(borrow.from);
        }

        borrows.clear();
        getBorrowsByDestRootedAt(base_var, &borrows);
        for (const Borrow& borrow : borrows) {
            cpath_set->insert(borrow.to);
        }
    }

    inline std::string rename_transformed_cpath_back(std::string cpath_str) {
        size_t cpath_str_len = cpath_str.length();
        for (int i=0; i < cpath_str_len; i++) {
            char cur_char = cpath_str[i];
            if (cur_char == '<') {cpath_str[i] = '[';}
            else if (cur_char == '>') {cpath_str[i] = ']';}
            else if (cur_char == '!') {cpath_str[i] = '.';}
            else if (cur_char == '+') {cpath_str[i] = '*';}
        }
        return cpath_str;
    }

    inline CPath rename_cpath_before_func_call(const CPath& orig_cpath, std::set<CPathPart>& arg_names, std::map<CPath, CPath>* rename_map, const std::string& prefix) {
        if (arg_names.count(orig_cpath[0]) == 0) {
            return orig_cpath;
        }
        bool has_deref = false;
        for (CPathPart part : orig_cpath) {
            if (part == DEREF_PART_ID) {
                has_deref = true;
                break;
            }
        }
        if (!has_deref) {
            return orig_cpath;
        }
        // Motivating example:
        //  - Consider a callsite of a function foo(int*** arg_1, int*** arg_2, int*** arg_3).
        //  - The callee (foo) may write to *arg_i and **arg_i.
        //  - If the function summary of foo indicates that foo makes borrows from the
        //    original values of arg referents, we should distinguish between the original
        //    values and the new values when updating the BorrowInfo after the callsite.
        // For each C-path rooted at an argument, we create a dummy variable to hold the original
        // value stored at the C-path location.  We take the original C-path string and replace
        // special characters to ensure that the new C-path is parsed as single variable.
        // This is similar to what we do with function arguments at function entry (search for ":@orig").
        std::string cpath_str = cpath_to_str(orig_cpath, false);
        size_t cpath_str_len = cpath_str.length();
        for (int i=0; i < cpath_str_len; i++) {
            char cur_char = cpath_str[i];
            if (cur_char == '[') {cpath_str[i] = '<';}
            else if (cur_char == ']') {cpath_str[i] = '>';}
            else if (cur_char == '.') {cpath_str[i] = '!';}
            else if (cur_char == '*') {cpath_str[i] = '+';}
        }
        CPath new_cpath = (CPath){get_cpath_part_id_from_str(prefix + cpath_str)};
        (*rename_map)[orig_cpath] = new_cpath;
        return new_cpath;
    }

    void rename_borrows_before_func_call(std::set<CPathPart>& arg_names, std::map<CPath, CPath>* rename_map, const std::string& prefix) {
        std::vector<Borrow> del_borrows;
        std::vector<Borrow> add_borrows;
        for (const Borrow& borrow : curBI.borrows_by_src) {
            CPath new_to   = rename_cpath_before_func_call(borrow.to,   arg_names, rename_map, prefix);
            CPath new_from = rename_cpath_before_func_call(borrow.from, arg_names, rename_map, prefix);
            if (new_to != borrow.to || new_from != borrow.from) {
                del_borrows.push_back(borrow);
                add_borrows.push_back((Borrow){.to = new_to, .from = new_from});
            }
        }
        for (Borrow& borrow : del_borrows) {
            curBI.borrows_by_src.erase(borrow);
            curBI.borrows_by_dest.erase(borrow);
        }
        for (Borrow& borrow : add_borrows) {
            curBI.borrows_by_src.insert(borrow);
            curBI.borrows_by_dest.insert(borrow);
        }
    }

    CPathPart rename_dead_basevar_in_borrows(CPathPart base_var) {
        std::string base_var_str = cpath_part_id_to_str[base_var];
        CPathPart dead_base_var = get_cpath_part_id_from_str("dead:" + base_var_str);
        cpathToValue["dead:" + base_var_str] = cpathToValue[base_var_str];
        std::vector<Borrow> del_borrows;
        std::vector<Borrow> add_borrows;
        for (const Borrow& borrow : curBI.borrows_by_src) {
            bool dirty = false;
            CPath new_to = borrow.to;
            if (new_to[0] == base_var) {
                new_to[0] = dead_base_var;
                dirty = true;
            }
            CPath new_from = borrow.from;
            if (new_from[0] == base_var) {
                new_from[0] = dead_base_var;
                dirty = true;
            }
            if (dirty) {
                del_borrows.push_back(borrow);
                add_borrows.push_back((Borrow){.to = new_to, .from = new_from});
            }
        }
        for (Borrow& borrow : del_borrows) {
            curBI.borrows_by_src.erase(borrow);
            curBI.borrows_by_dest.erase(borrow);
        }
        for (Borrow& borrow : add_borrows) {
            curBI.borrows_by_src.insert(borrow);
            curBI.borrows_by_dest.insert(borrow);
        }
        return dead_base_var;
    }
    
    void processLoad(LoadInst &LI) {
        Value *ptr = LI.getPointerOperand();
        std::string p1Name = getCPath(*ptr);
        std::string tName = getCPath(LI);
        std::string stmtPre = getStatementLabel(LI, "pre");
        std::string stmtPost = getStatementLabel(LI, "post");
        
        if (!isDryRun) {
            if (!DisableNullCheck) {
                // !null(p1, S-pre)
                outFile << "!null(" << p1Name << ", " << stmtPre << ") # Cannot read via a NULL ptr.\n"; /* pri_cat=null_deref */
            }
            // !zombie(p1, S-pre)
            outFile << "!zombie(" << p1Name << ", " << stmtPre << ") # Cannot read via a zombie ptr.\n"; /* pri_cat=deref_zombie */
            outFile << "good(" << p1Name << ", " << stmtPre << ") # Cannot read via a non-good ptr.\n"; /* pri_cat=deref_zombie */

            // !mut(p1) -> !mut(t)
            outFile << "mut(" << p1Name << ") | !mut(" << tName << ") # Cannot get mutable access via an immutable ptr.\n"; /* pri_cat=mut_via_immut */
            
            if (!DisableBorrowChecks && !isDryRun) {
                std::vector<Borrow> borrows;
                getBorrowsBySrcRootedAt(parse_cpath(p1Name), &borrows);
                if (borrows.size() > 0) {
                    outFile << "!responsible(" << tName << ") # Cannot transfer responsibility via a ptr (" << p1Name << ") that has outstanding borrows.  Borrowed by:"; /* pri_cat=borrow */
                    for (Borrow& borrow : borrows) {
                        outFile << " " << cpath_to_str(borrow.to);
                    }
                    outFile << "\n";
                }
                for (Borrow& borrow : borrows) {
                    CPath borrower = borrow.to;
                    outFile << "!mut(" << cpath_to_str(borrower) << ") # Cannot read via a ptr (" << p1Name << ") that is mutably borrowed.\n"; /* pri_cat=borrow */
                }
            }
        }
        add_seen_cpath_str(p1Name);
        
        std::set<std::string> changedCPaths = {};
        std::set<std::string> maybeZombifiedCPaths = {};
        
        if (LI.getType()->isPointerTy()) {
            // Use PtrCopyConstraints for pointer loads
            std::string derefP1 = "*" + p1Name;
            
            add_seen_cpath_str(tName);
            add_seen_cpath_str(derefP1);
            changedCPaths.insert(tName);

            if (!DisableBorrowChecks) {
                std::vector<CPath> aliases;
                get_loc_aliases_of_possible_deref(parse_cpath(derefP1), false, &aliases);
                
                for (const CPath& alias_cpath : aliases) {
                    std::string ali = cpath_to_str(alias_cpath);
                    maybeZombifiedCPaths.insert(ali);
                }
            }
            
            PtrCopyArgs ptrCopyArgs = {
                .dest = tName,
                .src = derefP1,
                .stmtPre = stmtPre,
                .stmtPost = stmtPost,
                .stmtPostDest = stmtPost,
                .inst = &LI
            };
            PtrCopyConstraints(ptrCopyArgs);
        }

        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, maybeZombifiedCPaths);
    }

    std::vector<std::string> get_unsupported_feats_metadata(Instruction* I) {
        MDNode *MD = I->getMetadata("unsupported_features");
        if (!MD || MD->getNumOperands() == 0) {
            return {};
        }
        
        // Parse metadata to get list of possible callees
        std::vector<std::string> ret;
        
        for (unsigned i = 0; i < MD->getNumOperands(); ++i) {
            MDString *nameMD = dyn_cast<MDString>(MD->getOperand(i));
            if (!nameMD) continue;
            ret.push_back(nameMD->getString().str());
        }
        return ret;
    }

    void processStore(StoreInst &SI) {
        Value *ptr = SI.getPointerOperand();
        Value *val = SI.getValueOperand();
        std::string p1Name = getCPath(*ptr);
        std::string stmtPre = getStatementLabel(SI, "pre");
        std::string stmtPost = getStatementLabel(SI, "post");
        
        if (val->getType()->isPointerTy() && !isDryRun) {
            for (std::string feat : get_unsupported_feats_metadata(&SI)) {
                seen_unsupported.insert(feat);
            }
            if (isa<GlobalVariable>(ptr)) {
                seen_unsupported.insert("global_ptr_write");
            }
            if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(ptr)) {
                Type* sourceElementType = GEP->getSourceElementType();
                if (isa<StructType>(sourceElementType)) {
                    seen_unsupported.insert("write_ptr_to_struct");
                }
            }
        }

        if (!isDryRun) {
            // mut(p1)
            outFile << "mut(" << p1Name << ") # Ptr used for writing must be mut.\n"; /* pri_cat=mut */
            if (!DisableNullCheck) {
                // !null(p1, S-pre)
                outFile << "!null(" << p1Name << ", " << stmtPre << ") # Cannot write via a NULL ptr.\n"; /* pri_cat=null_deref */
            }
            // !zombie(p1, S-pre)
            outFile << "!zombie(" << p1Name << ", " << stmtPre << ") # Cannot write via a zombie ptr.\n"; /* pri_cat=deref_zombie */
            outFile << "good(" << p1Name << ", " << stmtPre << ") # Cannot read via a non-good ptr.\n"; /* pri_cat=deref_zombie */
        }
        std::vector<CPath> aliases;
        if (!DisableBorrowChecks && val->getType()->isPointerTy()) {
            /* Value currently at the pointed-to memory location gets killed. */
            //handleCPathBecomingDead(parse_cpath("*" + p1Name));
            getAliasesOf(parse_cpath(p1Name), true, &aliases);
            for (CPath& alias : aliases) {
                handleCPathBecomingDead(parse_cpath("*" + cpath_to_str(alias)));
            }
            check_on_waiting_to_die();
        }
        if (!isDryRun) {
            if (!DisableBorrowChecks) {
                std::vector<Borrow> borrows;
                getBorrowsBySrcRootedAt(parse_cpath(p1Name), &borrows);
                if (TargetDebugInstId == getInstructionId(SI)) {
                    errs() << "Aliases of " << p1Name << ": [";
                    for (CPath& alias : aliases) {
                        errs() << " " << cpath_to_str(alias);
                    }
                    errs() << " ]\n";
                    errs() << "\nInstruction " << getInstructionId(SI) << " after killing cpath " << cpath_to_str(parse_cpath("*" + p1Name)) << "\n";
                    errs() << dump_borrows_to_str(curBI);
                    errs() << "\n";
                }
                bool isFirst = true;
                //errs() << "Hello " << p1Name << "\n";
                //if (!isDryRun) {
                //    errs() << dump_borrows_to_str(curBI);
                //}
                if (borrows.size() > 0) {
                    outFile << "false # Cannot write via pointer (" << p1Name << ") that is borrowed.  Borrowed by: ";
                    for (Borrow& borrow : borrows) {
                        CPath borrower = borrow.to;
                        std::string borrower_name = cpath_to_str(borrower);
                        if (!isFirst) {
                            outFile << ", ";
                        } else {
                            isFirst = false;
                        }
                        outFile << borrower_name;
                        Value* otherValue = cpathToValue[borrower_name];
                        if (otherValue) {
                            if (Instruction* otherInst = dyn_cast<Instruction>(otherValue)) {
                                outFile << std::string(" (I") << getInstructionId(*otherInst) << ")";
                            }
                        }
                    }
                    outFile << "\n";
                }
            }
        }
        add_seen_cpath_str(p1Name);
        
        std::set<std::string> changedCPaths = {};
        std::set<std::string> maybeZombifiedCPaths = {};
        
        if (SI.getValueOperand()->getType()->isPointerTy()) {
            std::string tName = getCPath(*val);
            std::string derefP1 = "*" + p1Name;
            
            add_seen_cpath_str(derefP1);
            add_seen_cpath_str(tName);
            maybeZombifiedCPaths.insert(tName);
            changedCPaths.insert(derefP1);

            if (!DisableMemLeak && !isDryRun) {
                outFile << "!responsible(" << derefP1 << ") | !good(" << derefP1 << ", " << stmtPre << ") # Overwriting a good responsible pointer causes a memory leak\n"; /* pri_cat=mem_leak */
            }

            if (!DisableBorrowChecks) {
                std::vector<CPath> aliases;
                get_loc_aliases_of_possible_deref(parse_cpath(derefP1), true, &aliases);
                
                for (const CPath& alias_cpath : aliases) {
                    std::string ali = cpath_to_str(alias_cpath);
                    changedCPaths.insert(ali);
                }
            }
            
            // Use PtrCopyConstraints for pointer stores
            PtrCopyArgs ptrCopyArgs = {
                .dest = derefP1,
                .src = tName,
                .stmtPre = stmtPre,
                .stmtPost = stmtPost,
                .stmtPostDest = stmtPost,
                .inst = &SI
            };
            PtrCopyConstraints(ptrCopyArgs);
        }
        
        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, maybeZombifiedCPaths);
    }

    DILocalVariable* get_pom_orig_var_DIVar(Instruction &I) {
        MDNode *MD = I.getMetadata("pom_orig_var");
        if (!MD) {return nullptr;}
        
        DILocalVariable *DIVar = dyn_cast<DILocalVariable>(MD);
        return DIVar;
    }

    std::string get_pom_orig_var_name(Instruction &I) {
        DILocalVariable *DIVar = get_pom_orig_var_DIVar(I);
        if (!DIVar) {
            if (I.getMetadata("is_retval")) {
                return std::string("__retval");
            }
            return std::string("");
        }
        return DIVar->getName().str();
    }

    std::string get_phi_origin_metadata(llvm::PHINode *inst) {
        llvm::MDNode *MD = inst->getMetadata("phi_origin");
        if (!MD) {
            return "";
        }
        if (MD->getNumOperands() != 1) {
            return "";
        }
        llvm::Metadata *Op = MD->getOperand(0);
        if (llvm::MDString *MDS = llvm::dyn_cast<llvm::MDString>(Op)) {
            return MDS->getString().str();
        } else {
            return "";
        }
    }
    
    void processPomVarStore(CallInst &CI) {
        std::string varName = getCPath(CI);
        std::string stmtPre = getStatementLabel(CI, "pre");
        std::string stmtPost = getStatementLabel(CI, "post");
        std::string srcName = getCPath(*CI.getArgOperand(0));

        if (!has_nondummy_uses(&CI)) {
            //if (!isDryRun) {
            //    errs() << "Skipping PomVarStore I" << getInstructionId(CI) << " because it has no uses.\n";
            //}
            generatePreservationConstraints(stmtPre, stmtPost, {}, {});
            return;
        }

        PtrCopyArgs ptrCopyArgs = {
            .dest = varName,
            .src = srcName,
            .stmtPre = stmtPre,
            .stmtPost = stmtPost,
            .inst = &CI
        };
        PtrCopyConstraints(ptrCopyArgs);

        add_seen_cpath_str(varName);
        add_seen_cpath_str(srcName);
        std::set<std::string> changedCPaths = {varName};
        std::set<std::string> maybeZombifiedCPaths = {srcName};
        
        if (!isDryRun) {
            if (!DisablePomLocals) {
                std::string funcName = CI.getParent()->getParent()->getName().str();
                std::string origVarName = get_pom_orig_var_name(CI);
                if (origVarName != "") {
                    std::string pomName = funcName + "::locals::" + origVarName;
                    outFile << "!responsible(" << varName << ") | responsible(" << pomName << ")\n";
                    outFile << "!responsible(" << pomName << ") | responsible(" << varName << ")\n";
                    outFile << "!mut(" << varName << ") | mut(" << pomName << ")\n";
                    outFile << "!mut(" << pomName << ") | mut(" << varName << ")\n";
                }
            }
        }
        
        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, maybeZombifiedCPaths);
    }

    void processFreeze(Instruction &I) {
        std::string stmtPre = getStatementLabel(I, "pre");
        std::string stmtPost = getStatementLabel(I, "post");

        if (!I.getType()->isPointerTy()) {
            generatePreservationConstraints(stmtPre, stmtPost, {}, {});
            return;
        }

        std::string destName = getCPath(I);
        std::string srcName = getCPath(*I.getOperand(0));

        std::set<std::string> changedCPaths = {destName};
        std::set<std::string> maybeZombifiedCPaths = {};

        if (isa<UndefValue>(I.getOperand(0)) || isa<PoisonValue>(I.getOperand(0))) {
            if (!isDryRun) {
                outFile << "zombie(" << destName << ", " << stmtPost << ") # Undef/poison value\n";
            }
        } else {
            PtrCopyArgs ptrCopyArgs = {
                .dest = destName,
                .src = srcName,
                .stmtPre = stmtPre,
                .stmtPost = stmtPost,
                .inst = &I
            };
            PtrCopyConstraints(ptrCopyArgs);
            maybeZombifiedCPaths.insert(srcName);
        }

        add_seen_cpath_str(destName);
        add_seen_cpath_str(srcName);
        
        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, maybeZombifiedCPaths);
    }

    void processSelect(SelectInst &SI) {
        std::string stmtPre = getStatementLabel(SI, "pre");
        std::string stmtPost = getStatementLabel(SI, "post");

        if (!SI.getType()->isPointerTy()) {
            generatePreservationConstraints(stmtPre, stmtPost, {}, {});
            return;
        }

        std::string destName = getCPath(SI);
        std::string val1_Name = getCPath(*SI.getTrueValue());
        std::string val2_Name = getCPath(*SI.getFalseValue());

        PtrCopyArgs ptrCopyArgs = {
            .dest = destName,
            .src = val1_Name,
            .stmtPre = stmtPre,
            .stmtPost = stmtPost,
            .inst = &SI
        };
        PtrCopyConstraints(ptrCopyArgs);

        ptrCopyArgs.src = val2_Name;
        PtrCopyConstraints(ptrCopyArgs);

        add_seen_cpath_str(destName);
        add_seen_cpath_str(val1_Name);
        add_seen_cpath_str(val2_Name);
        std::set<std::string> changedCPaths = {destName};
        std::set<std::string> maybeZombifiedCPaths = {val1_Name, val2_Name};
        
        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, maybeZombifiedCPaths);
    }

    void processNop(Instruction &I) {
        std::string stmtPre = getStatementLabel(I, "pre");
        std::string stmtPost = getStatementLabel(I, "post");
        generatePreservationConstraints(stmtPre, stmtPost, {}, {});
    }
    
    void processPhi(PHINode &PN) {
        std::string stmtPre = getStatementLabel(PN, "pre");
        std::string stmtPost = getStatementLabel(PN, "post");

        if (!PN.getType()->isPointerTy()) {
            generatePreservationConstraints(stmtPre, stmtPost, {}, {});
            return;
        }

        std::string tName = getCPath(PN);
        std::set<std::string> maybeZombifiedCPaths = {};
        
        for (unsigned i = 0; i < PN.getNumIncomingValues(); ++i) {
            Value *incomingVal = PN.getIncomingValue(i);
            BasicBlock *incomingBB = PN.getIncomingBlock(i);
            std::string xiName = getCPath(*incomingVal);
            
            // Get the last instruction of the incoming basic block
            std::string liPost;
            if (!incomingBB->empty()) {
                Instruction &lastInst = incomingBB->back();
                Instruction* prev = lastInst.getPrevNode();
                while (prev && isDbgIntrinsic(*prev)) {
                    prev = prev->getPrevNode();
                }
                if (prev) {
                    liPost = getStatementLabel(*prev, "post");
                } else {
                    liPost = getStatementLabel(lastInst, "post");
                }
            } else {
                // Empty basic block - this shouldn't happen in valid LLVM IR
                errs() << "Error: Empty basic block in function " << curFunc->getName().str() << "!\n";
                liPost = "BB_" + std::to_string(reinterpret_cast<uintptr_t>(incomingBB)) + "-post";
            }
            
            if (!isDryRun) {
                outFile << "# Phi incoming: " + liPost + "\n";
            }

            // Use PtrCopyConstraints for each incoming value
            PtrCopyArgs ptrCopyArgs = {
                .dest = tName,
                .src = xiName,
                .stmtPre = stmtPre, //liPost,
                .stmtPost = stmtPost,
                .stmtPostDest = stmtPost,
                .inst = &PN
            };
            PtrCopyConstraints(ptrCopyArgs);
            
            maybeZombifiedCPaths.insert(xiName);
        }
        
        add_seen_cpath_str(tName);

        std::set<std::string> changedCPaths = {tName};
        generatePreservationConstraints(stmtPre, stmtPost, changedCPaths, maybeZombifiedCPaths);
    }
    
    void processReturn(ReturnInst &RI) {
        std::string stmtPre = getStatementLabel(RI, "pre");
        std::string stmtPost = getStatementLabel(RI, "post"); // This will be "end"
        Value *retVal = RI.getReturnValue();
        static std::unordered_set<std::string> already_warned;

        if (retVal && retVal->getType()->isPointerTy()) {
            // There is a return value
            std::string tName = getCPath(*retVal);
            
            // Add to seen C-paths
            add_seen_cpath_str(tName);

            std::string curFuncName = RI.getParent()->getParent()->getName().str();
            std::string returnName = curFuncName + "::return";
            add_seen_cpath_str(returnName);

            PtrCopyArgs ptrCopyArgs = {
                .dest = returnName,
                .src = tName,
                .stmtPre = stmtPre,
                .stmtPost = stmtPost,
                .stmtPostDest = stmtPost,
                .inst = &RI,
            };
            handleBorrowsAtPtrCopy(ptrCopyArgs);
            
            std::string starPfx = ""; // Start with no stars in this case.
            int stopDepth = getDeepPtrCopyStopDepth(returnName, tName);
            bool haveRealStopDepth = (stopDepth != MAX_PTR_DEPTH);
            while (true) {
                if (haveRealStopDepth || starPfx == std::string("") || seenCPaths.count(starPfx + tName)) {
                    if (starPfx.length() > stopDepth) {
                        if (stopDepth == MAX_PTR_DEPTH) {
                            errs() << "WARNING: pointer depth is too large, not going any further!\n";
                        }
                        break;
                    }
                    
                    if (!pomCPaths.count(starPfx + returnName) && !already_warned.count(starPfx + returnName)) {
                        pomMissingErrors.push_back("Missing in '.pom.yml' file: " + starPfx + returnName);
                    }

                    add_seen_cpath_str(starPfx + tName);
                    add_seen_cpath_str(starPfx + returnName);

                    if (!isDryRun) {
                        // responsible(t) <-> responsible(return)
                        outFile << "!responsible(" << starPfx << tName << ") | responsible(" << starPfx << returnName << ")\n";
                        outFile << "!responsible(" << starPfx << returnName << ") | responsible(" << starPfx << tName << ")\n";

                        // good(t, S-pre) -> good(containing_func::return, end), and ditto for null and zombie
                        outFile << "!good(" << starPfx << tName << ", " << stmtPre << ") | good(" << starPfx << returnName << ", " << stmtPost << ")\n";
                        outFile << "!null(" << starPfx << tName << ", " << stmtPre << ") | null(" << starPfx << returnName << ", " << stmtPost << ")\n";
                        outFile << "!zombie(" << starPfx << tName << ", " << stmtPre << ") | zombie(" << starPfx << returnName << ", " << stmtPost << ")\n";

                        // mut(containing_func::return) -> mut(t)
                        outFile << "!mut(" << starPfx << curFuncName << "::return) | mut(" << starPfx << tName << ")\n";
                    }

                    starPfx += "*";
                } else {
                    break;
                }
            }
            
            // Generate preservation constraints for other variables
            generatePreservationConstraints(stmtPre, stmtPost, {tName}, {});
        } else {
            // void return, no return value
            // Just generate preservation constraints for all variables
            generatePreservationConstraints(stmtPre, stmtPost, {}, {});
        }
    }

    
    Value* trace_thru_GEP(Value* ptr) {
        while (true) {
            if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(ptr)) {
                ptr = GEP->getPointerOperand();
            } else {
                break;
            }
        }
        return ptr;
    }

    Function* getCalledFunc(CallBase* CB) {
        return dyn_cast<Function>(CB->getCalledOperand()->stripPointerCasts());
    }

    void processGEP(GetElementPtrInst &GEP) {
        std::string tName = getCPath(GEP);
        std::string stmtPost = getStatementLabel(GEP, "post");
        std::string stmtPre = getStatementLabel(GEP, "pre");

        Type* sourceElementType = GEP.getSourceElementType();
        bool isStruct = isa<StructType>(sourceElementType);
        std::string base_ptr = getCPath(*GEP.getPointerOperand());
        std::string src = base_ptr;
        if (isStruct) {
            // GEP for calculating address of a field of a struct
            std::string structName, fieldName;
            std::tie(structName, fieldName) = get_GEP_field_info(&GEP);
            if (structName.empty() || fieldName.empty()) {
                goto missing_struct_info;
            }
            //outs() << "Struct '" << structName << "', field '" << fieldName << "'\n";
            if (!isDryRun && !DisablePomStructs) {
                std::string pomFieldName = "struct::" + structName + "::" + fieldName;
                // Note that GEP returns the address of the field, not the value of the field.
                outFile << "!responsible(*" << tName << ") | responsible(" << pomFieldName << ")\n";
                outFile << "!responsible(" << pomFieldName << ") | responsible(*" << tName << ")\n";
                outFile << "!mut(*" << tName << ") | mut(" << pomFieldName << ")\n";
                outFile << "!mut(" << pomFieldName << ") | mut(*" << tName << ")\n";
            }
            src = base_ptr + "." + fieldName;
            missing_struct_info:;
        } else {
            // GEP for calculating address of element of array.
        }
        if (!DisableBorrowChecks) {
            PtrCopyArgs ptrCopyArgs = {
                .dest = tName,
                .src = src,
                .stmtPre = stmtPre,
                .stmtPost = stmtPost,
                .stmtPostDest = stmtPost,
                .inst = &GEP,
            };
            PtrCopyConstraints(ptrCopyArgs);
        }

        // !responsible(t)
        if (!isDryRun) {
            std::string base_ptr = getCPath(*GEP.getPointerOperand());
            outFile << "!responsible(" << tName << ") # GetElementPtr\n";
            outFile << "!mut(" << tName << ") | mut(" << base_ptr << ") # GetElementPtr\n";
        }
    
        // Add to seen C-paths
        add_seen_cpath_str(tName);
        
        // good(x, S-post) <-> good(x, S-pre) for all other x (except t)
        generatePreservationConstraints(stmtPre, stmtPost, {tName}, {});
    }

    #define BRANCH_WHERE_VAR_IS_NULL 0
    #define BRANCH_WHERE_VAR_IS_NOT_NULL 1

    void processControlFlow(BasicBlock &BB) {
        if (BB.empty()) return;
        
        // Get the last instruction of this basic block
        Instruction &lastInst = BB.back();
        std::string lastInstPost = getStatementLabel(lastInst, "post");
        
        for (BasicBlock *Succ : successors(&BB)) {
            if (Succ->empty()) continue;
            
            // Get the first instruction of the successor basic block
            Instruction &firstInst = *get_first_real_inst_of_BB(*Succ);
            std::string firstInstPre = getStatementLabel(firstInst, "pre");
            
            // Check if this is a conditional jump
            bool isNullCondJump = false;
            Value* varComparedToNull = nullptr;
            int conditionCode = 0;
            
            if (BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator())) {
                if (BI->isConditional()) {
                    
                    // Get the condition
                    Value *condition = BI->getCondition();
                    
                    // Check if it's a comparison instruction
                    if (ICmpInst *icmp = dyn_cast<ICmpInst>(condition)) {
                        Value *op0 = icmp->getOperand(0);
                        Value *op1 = icmp->getOperand(1);
                        
                        // Check if one operand is null and the other is a pointer
                        Value *ptrOperand = nullptr;
                        isNullCondJump = false;
                        
                        if (isa<ConstantPointerNull>(op0) && op1->getType()->isPointerTy()) {
                            ptrOperand = op1;
                            isNullCondJump = true;
                        } else if (isa<ConstantPointerNull>(op1) && op0->getType()->isPointerTy()) {
                            ptrOperand = op0;
                            isNullCondJump = true;
                        }
                        
                        if (isNullCondJump) {
                            varComparedToNull = ptrOperand;
                            
                            // Determine which branch this successor represents
                            ICmpInst::Predicate pred = icmp->getPredicate();
                            bool trueSuccessor = (Succ == BI->getSuccessor(0));
                            
                            if (pred == ICmpInst::ICMP_EQ) {
                                // ptr == null
                                conditionCode = trueSuccessor ? BRANCH_WHERE_VAR_IS_NULL : BRANCH_WHERE_VAR_IS_NOT_NULL;
                            } else if (pred == ICmpInst::ICMP_NE) {
                                // ptr != null
                                conditionCode = trueSuccessor ? BRANCH_WHERE_VAR_IS_NOT_NULL : BRANCH_WHERE_VAR_IS_NULL;
                            } else {
                                isNullCondJump = false;
                            }
                        }
                    }
                }
            }
            
            // Generate flow constraints from last instruction of BB to first instruction of successor
            generateFlowConstraints(lastInst, firstInst, isNullCondJump, varComparedToNull, conditionCode);
        }
    }

    bool has_nonself_borrows(Instruction* inst) {
        std::vector<Borrow> borrows;
        getBorrowsBySrcRootedAt(parse_cpath(getCPath(*inst)), &borrows);
        DILocalVariable* self_name_DI = get_pom_orig_var_DIVar(*inst);
        for (const Borrow& borrow : borrows) {
            if (borrow.from.size() > 1) {
                return true;
            }
            Instruction* to = dyn_cast<Instruction>(cpathToValue[cpath_part_id_to_str[borrow.from[0]]]);
            if (!to) {
                return true;
            }
            if (get_pom_orig_var_DIVar(*to) != self_name_DI) {
                return true;
            }
        }
        return false;
    }

    void handleBorrowsAtPtrCopy(PtrCopyArgs& args) {

        std::string& srcName = args.src;
        std::string& destName = args.dest;

        CPath destCPath = parse_cpath(destName);
        CPath srcCPath = parse_cpath(srcName);

        if (srcName == destName) {
            return;
        }

        if (DisableBorrowChecks || srcName == "NULL_CONST") {
            return;
        }

        CPathPart src_base = srcCPath[0];
        std::string src_base_str = cpath_part_id_to_str[src_base];
        bool hasExistingBorrow = false;
        std::string otherBorrowing;
        std::string allOtherBorrowing;

        //CPath src_plus_max = srcCPath;
        CPath src_base_path = (CPath){srcCPath[0]};
        CPath src_plus_max = src_base_path;
        src_plus_max.push_back(CPathPartMax);
        const Borrow lower_borrow = (Borrow){.to = (CPath){0}, .from = src_base_path};
        const Borrow upper_borrow = (Borrow){.to = (CPath){0}, .from = src_plus_max};
        auto itLo = curBI.borrows_by_src.lower_bound(lower_borrow);
        auto itHi = curBI.borrows_by_src.lower_bound(upper_borrow);
        unsigned srcInstId = 0;
        Value* srcLLVM = cpathToValue[src_base_str];
        Instruction* srcInst;
        if (srcLLVM) {
            srcInst = dyn_cast<Instruction>(srcLLVM);
            if (srcInst) {
                srcInstId = getInstructionId(*srcInst);
            }
        }
        for (auto it = itLo; it != itHi; ++it) {
            otherBorrowing = cpath_to_str((*it).to);
            if (isa<PHINode>(args.inst) && srcInst) {
                Value* otherLLVM = cpathToValue[otherBorrowing];
                if (otherLLVM && isa<PHINode>(otherLLVM)) {
                    DILocalVariable* src_orig_name_DI = get_pom_orig_var_DIVar(*srcInst);
                    bool ok = (
                        src_orig_name_DI == get_pom_orig_var_DIVar(*args.inst) &&
                        src_orig_name_DI == get_pom_orig_var_DIVar(*dyn_cast<PHINode>(otherLLVM)) &&
                        !has_nonself_borrows(srcInst) &&
                        !has_nonself_borrows(dyn_cast<PHINode>(otherLLVM)));
                    if (ok) {
                        continue;
                    }
                }
            }
            hasExistingBorrow = true;
            if (!allOtherBorrowing.empty()) {
                allOtherBorrowing += ", ";
            }
            allOtherBorrowing += otherBorrowing;
            Value* otherValue = cpathToValue[otherBorrowing];
            if (otherValue) {
                if (Instruction* otherInst = dyn_cast<Instruction>(otherValue)) {
                    allOtherBorrowing += (std::string(" (I") + std::to_string(getInstructionId(*otherInst)) + ")");
                }
            }
            if (curBI.get_is_dyn_exclusive(src_base, true)) {
                if (!isDryRun) {
                    outFile << "!mut(" << otherBorrowing << ") # Another borrow rooted at " << src_base_str << " (I" << srcInstId << ")" << " now exists: " << destName << "\n"; /* pri_cat=borrow */
                }
            } else {
                // The "!mut" constraints for existing borrows would have already been generated
                // when is_dyn_exclusive became false, so we don't need to generate them again here.
            }
        }
        curBI.is_dyn_exclusive[src_base] = hasExistingBorrow ? false : true;

        Borrow new_borrow = (Borrow){.to = destCPath, .from = srcCPath};

        if (!isDryRun) {
            ++debugPoint;
            if (TargetDebugPoint == debugPoint) {
                errs() << "\nDebug point " << debugPoint << " (I" << getInstructionId(*args.inst) << "):\n";
                errs() << dump_borrows_to_str(curBI);
                errs() << "\n";
            }
        }
        if (hasExistingBorrow) {
            if (!isDryRun) {
                //errs() << dump_borrows_to_str(curBI);
                outFile << "!mut(" << destName << ") # Another borrow rooted at " << src_base_str << " (I" << srcInstId << ")" << " already exists: " << allOtherBorrowing << " # New borrow is from " << srcName << " to " << destName << " # DebugPoint=" << (debugPoint) << "\n"; /* pri_cat=borrow */
            }
        }
        
        if (true) {
            //if (!isDryRun) {
            //    errs() << "I" << getInstructionId(*args.inst) << ": New borrow from " << srcName << " to " << destName << "\n";
            //}
            bool makes_cycle = false;
            add_borrow(new_borrow, &makes_cycle);
            if (makes_cycle) {
                errs() << "(during instruction " << getInstructionId(*args.inst) << ")\n\n";
            }
            curBI.is_must_borrow[new_borrow] = true;
            //if (!isDryRun) {
            //    errs() << dump_borrows_to_str(curBI);
            //}
        }
    }

    void get_loc_aliases_of_possible_deref(CPath givenCPath, bool only_must, std::vector<CPath>* loc_aliases) {
        if (givenCPath.back() == DEREF_PART_ID && !DisableBorrowChecks) {
            CPath givenAddr = givenCPath;
            givenAddr.pop_back();
            getAliasesOf(givenAddr, only_must, loc_aliases);
            for (CPath& alias_cpath : *loc_aliases) {
                alias_cpath.push_back(DEREF_PART_ID);
            }
        } else {
            loc_aliases->push_back(givenCPath);
        }
    }

    void PtrCopyConstraints(PtrCopyArgs& args) {
        if (args.stmtPostDest.empty()) {
            args.stmtPostDest = args.stmtPost;
        }
        std::string& srcName = args.src;
        std::string& destName = args.dest;
        std::string& stmtPre = args.stmtPre;
        std::string& stmtPost = args.stmtPost;
        std::string& stmtPostDest = args.stmtPostDest;

        std::vector<CPath> dest_loc_aliases;
        std::vector<CPath> src_loc_aliases;
        std::vector<CPath> src_loc_must_alias_vec;

        get_loc_aliases_of_possible_deref(parse_cpath(destName), false, &dest_loc_aliases);
        get_loc_aliases_of_possible_deref(parse_cpath(srcName), false, &src_loc_aliases);
        get_loc_aliases_of_possible_deref(parse_cpath(srcName), true, &src_loc_must_alias_vec);
        std::set<CPath> src_loc_must_alias_set(src_loc_must_alias_vec.begin(), src_loc_must_alias_vec.end());


        handleBorrowsAtPtrCopy(args);

        if (!isDryRun) {
            /* Type properties */
            // responsible(dest) -> responsible(src)
            outFile << "!responsible(" << destName << ") | responsible(" << srcName << ") # Resp ptr cannot acquire value from irresp ptr.\n"; /* pri_cat=resp_from_irresp */

            // mut(dest) -> mut(src)
            outFile << "!mut(" << destName << ") | mut(" << srcName << ")\n";
            
            /* POM constraint */
            // !responsible(dest) -> !zombie(src, S-pre)
            outFile << "responsible(" << destName << ") | !zombie(" << srcName << ", " << stmtPre << ") # Cannot assign zombie value to irresp ptr.\n"; /* pri_cat=copy_zombie */

            /* States properties of the source */
            for (int i=0; i < src_loc_aliases.size(); i++) {
                CPath& alias_cpath = src_loc_aliases[i];
                bool is_must = src_loc_must_alias_set.count(alias_cpath);
                std::string src_ali = cpath_to_str(alias_cpath);

                // responsible(dest) -> (good(src, S-pre) -> zombie(src, S-post))
                outFile << "!responsible(" << destName << ") | !good(" << src_ali << ", " << stmtPre << ") | zombie(" << src_ali << ", " << stmtPost << ") # Move semantics: copying a resp ptr from src to dest makes src a zombie.\n"; /* pri_cat=move_resp */
                
                if (is_must) {
                    // (!responsible(dest) && good(src, S-pre)) -> good(src, S-post)
                    outFile << "responsible(" << destName << ") | !good(" << src_ali << ", " << stmtPre << ") | good(" << src_ali << ", " << stmtPost << ") # Conditional preservation\n"; /* pri_cat=flow */
                } else {
                    // good(src, S-pre) -> good(src, S-post)
                    outFile << "!good(" << src_ali << ", " << stmtPre << ") | good(" << src_ali << ", " << stmtPost << ") # Uncertain potential location alias of memory location being read.\n";
                }

                // null(src, S-pre) -> null(src, S-post)
                outFile << "!null(" << src_ali << ", " << stmtPre << ") | null(" << src_ali << ", " << stmtPost << ") # Preservation\n";
                // zombie(src, S-pre) -> zombie(src, S-post)
                outFile << "!zombie(" << src_ali << ", " << stmtPre << ") | zombie(" << src_ali << ", " << stmtPost << ") # Preservation\n";
            }

            /* State properties of the destination */
            bool is_alias = false;
            std::string callsite = (isa<CallInst>(args.inst) ? "Callsite: " : "");
            for (CPath& alias_cpath : dest_loc_aliases) {
                std::string ali = cpath_to_str(alias_cpath);
                std::string is_ali_str = is_alias ? " (alias)" : " ";
                // (responsible(dest) && good(src, S-pre)) -> good(dest, S-post-dest)
                outFile << "!good(" << srcName << ", " << stmtPre << ") | good(" << ali << ", " << stmtPostDest << ") # " << callsite << "PtrCopy" + is_ali_str + "\n";
                // null(src, S-pre) -> null(dest, S-post-dest)
                outFile << "!null(" << srcName << ", " << stmtPre << ") | null(" << ali << ", " << stmtPostDest << ") # " << callsite << "PtrCopy" + is_ali_str + "\n";
                // (responsible(dest) && zombie(src, S-pre)) -> zombie(dest, S-post-dest)
                outFile << "!zombie(" << srcName << ", " << stmtPre << ") | zombie(" << ali << ", " << stmtPostDest << ") # " << callsite << "PtrCopy" + is_ali_str + "\n";
                bool is_alias = true;
            }
        }

        //std::vector<CPath> fake = {parse_cpath(args.dest)};
        //DeepPtrCopyConstraints(args, fake);
        DeepPtrCopyConstraints(args, dest_loc_aliases);
    }

    int getDeepPtrCopyStopDepth(std::string dest, std::string src) {
        static std::unordered_map<std::string, int> cache;
        int stopDepth = MAX_PTR_DEPTH;
        for (std::string src_or_dest : {src, dest}) {
            if (src_or_dest == std::string("NULL_CONST")) {
                return 0;
            }
            if (has_suffix(src_or_dest, "::return")) {
                assert(src_or_dest[0] != '*');
                std::string calleeName = strip_suffix(src_or_dest, strlen("::return"));
                Module* M = curFunc->getParent();
                int retDepth = getReturnTypePtrDepth(M->getFunction(calleeName));
                if (retDepth > 0 && retDepth <= stopDepth) {
                    stopDepth = retDepth - 1;
                }
            }
            size_t pos = src_or_dest.find("::args::");
            if (pos != std::string::npos) {
                auto it = cache.find(src_or_dest);
                int retDepth = MAX_PTR_DEPTH;
                if (it != cache.end()) {
                    retDepth = it->second;
                } else {
                    std::string callee_name = src_or_dest.substr(0, pos);
                    std::string arg_name = src_or_dest.substr(pos + 8);  // 8 == strlen("::args::")
                    Module* M = curFunc->getParent();
                    retDepth = getArgTypePtrDepth(M->getFunction(callee_name), arg_name);
                    // errs() << "ptr_depth_cache[\"" << src_or_dest << "\"] = " << retDepth << "\n";
                    cache.insert({src_or_dest, retDepth});
                }
                if (retDepth > 0 && retDepth <= stopDepth) {
                    stopDepth = retDepth - 1;
                }
            }
        }
        return stopDepth;
    }

    void DeepPtrCopyConstraints(PtrCopyArgs& args, std::vector<CPath>& dest_loc_aliases) {
        std::string& srcName = args.src;
        std::string& destName = args.dest;
        std::string& stmtPre = args.stmtPre;
        std::string& stmtPostDest = args.stmtPostDest;
        std::string starPfx = "*";
        
        int stopDepth = getDeepPtrCopyStopDepth(destName, srcName);
        bool haveRealStopDepth = (stopDepth != MAX_PTR_DEPTH);
        std::string callsite = (isa<CallInst>(args.inst) ? "Callsite: " : "");
        
        while (true) {
            if (haveRealStopDepth || seenCPaths.count(starPfx + destName) || seenCPaths.count(starPfx + srcName)) {
                if (starPfx.length() > stopDepth) {
                    if (stopDepth == MAX_PTR_DEPTH) {
                        errs() << "WARNING: pointer depth is too large, not going any further (inst=" << getInstructionId(*args.inst) << ", dest=" << destName << ", src=" << srcName << ")\n";
                    }
                    break;
                }

                if (!isDryRun) {
                    // responsible(*src) <-> responsible(*dest)
                    outFile << "!responsible(" << starPfx << srcName << ") | responsible(" << starPfx << destName << ") # " << callsite << "DeepPtrCopy\n";
                    outFile << "responsible(" << starPfx << srcName << ") | !responsible(" << starPfx << destName << ") # " << callsite << "DeepPtrCopy\n";
                    
                    // mut(*dest) -> mut(*src)
                    outFile << "!mut(" << starPfx << destName << ") | mut(" << starPfx << srcName << ") # " << callsite << "DeepPtrCopy\n";

                    for (CPath& alias_cpath : dest_loc_aliases) {
                        std::string dest_ali = cpath_to_str(alias_cpath);
                        // good(*src, S-pre) -> good(*dest, S-post-dest) and ditto for null and zombie
                        outFile << "!good(" << starPfx << srcName << ", " << stmtPre << ") | good(" << starPfx << dest_ali << ", " << stmtPostDest << ") # " << callsite << "DeepPtrCopy\n";
                        outFile << "!null(" << starPfx << srcName << ", " << stmtPre << ") | null(" << starPfx << dest_ali << ", " << stmtPostDest << ") # " << callsite << "DeepPtrCopy\n";
                        outFile << "!zombie(" << starPfx << srcName << ", " << stmtPre << ") | zombie(" << starPfx << dest_ali << ", " << stmtPostDest << ") # " << callsite << "DeepPtrCopy\n";
                    }
                }

                //if (seenCPaths.count(starPfx + destName) == 0) {
                //    errs() << "Inst " << getInstructionId(*args.inst) << ": adding cpath " << (starPfx + destName) << "\n";
                //}
                //if (seenCPaths.count(starPfx + srcName) == 0) {
                //    errs() << "Inst " << getInstructionId(*args.inst) << ": adding cpath " << (starPfx + srcName) << "\n";
                //}
                
                add_seen_cpath_str(starPfx + destName);
                add_seen_cpath_str(starPfx + srcName);

                starPfx += "*";
            } else {
                break;
            }
        }
    }

    Value* trace_thru_pom_var_store(Value* var) {
        CallInst* CI = dyn_cast<CallInst>(var);
        if (!CI) {
            return var;
        }
        Function *callee = CI->getCalledFunction();
        if (callee->getName() != "__pom_var_store") {
            return var;
        }
        if (CI->arg_size() != 1) {
            return var;
        }
        return CI->getArgOperand(0);
    }

    bool isReallocInst(Value* inst) {
        CallInst* CI = dyn_cast<CallInst>(inst);
        if (!CI) {
            return false;
        }
        Function *callee = CI->getCalledFunction();
        return (callee->getName() == "realloc");
    }

    void generateFlowConstraints(Instruction &fromInst, Instruction &toInst, 
                               bool isNullCondJump = false, /* whether this is jump conditioned on a nullness check */
                               Value* varComparedToNull = nullptr, 
                               int conditionCode = 0 /* one of BRANCH_WHERE_VAR_{IS_NULL,IS_NOT_NULL} */
    ) {
        std::string fromPost = getStatementLabel(fromInst, "post");
        std::string toPre = getStatementLabel(toInst, "pre");
        
        std::string condPtr;
        std::vector<CPath> aliases_list;
        std::set<CPath> aliases_set;
        Value* origAllocPtr = nullptr;
        std::string origAllocPtrCPath;
        std::vector<CPath> origAllocPtrAliasVec;
        std::set<std::string> origAllocPtrAliasSet;
        Value* reallocInst = nullptr;
        //if (isa<BranchInst>(&fromInst) && !isDryRun) {
        //    errs() << "Branch I" << getInstructionId(fromInst) << ": ";
        //    fromInst.dump();
        //}
        if (varComparedToNull != nullptr) {
            condPtr = getCPath(*varComparedToNull);
            getAliasesOf(parse_cpath(condPtr), false, &aliases_list);
            for (CPath alias : aliases_list) {
                aliases_set.insert(alias);
            }
            reallocInst = trace_thru_pom_var_store(varComparedToNull);
            if (isReallocInst(reallocInst)) {
                origAllocPtr = dyn_cast<CallInst>(reallocInst)->getArgOperand(0);
                origAllocPtrCPath = getCPath(*origAllocPtr);
                getAliasesOf(parse_cpath(origAllocPtrCPath), true, &origAllocPtrAliasVec);
                //if (!isDryRun) {
                //    errs() << "Instruction I" << getInstructionId(fromInst) << ":\n";
                //    errs() << "Got origAllocPtr: " << origAllocPtrCPath << "\n";
                //    varComparedToNull->dump();
                //    reallocInst->dump();
                //}
                for (CPath ali : origAllocPtrAliasVec) {
                    //if (!isDryRun) {
                    //    errs() << "  alias: " << cpath_to_str(ali) << "\n";
                    //}
                    origAllocPtrAliasSet.insert(cpath_to_str(ali));
                }
            } else {
                //if (!isDryRun) {
                //    errs() << "No realloc!\n";
                //    varComparedToNull->dump();
                //    reallocInst->dump();
                //}
                reallocInst = nullptr;
            }
            //if (!isDryRun) {
            //    errs() << "Borrows: \n" << dump_borrows_to_str(curBI) << "\n";
            //    errs() << "Aliases of " << condPtr << ": [";
            //    for (CPath alias : aliases_list) {
            //        errs() << " " << cpath_to_str(alias);
            //    }
            //    errs() << "]\n";
            //}
        }
        
        // For each seen C-path, generate flow constraints
        for (const std::string &cpath : seenCPaths) {
            if (has_suffix(cpath, "::return")) {
                continue;
            }
            bool is_arg_of_called_func = (cpath.find(std::string("::args::")) != std::string::npos);
            if (is_arg_of_called_func) {
                continue;
            }
            if (!isDryRun) {
                std::string base_path = cpath;
                while (true) {
                    if (base_path[0] == '*') {
                        base_path = base_path.substr(1);
                        continue;
                    } else {
                        break;
                    }
                }
                if (!DisableLiveness) {
                    Value* baseVar = cpathToValue[base_path];
                    if (baseVar != nullptr && baseVar != &fromInst && base_path != std::string("NULL_CONST") && currentLiveVars.count(baseVar) == 0) {
                        continue;
                    }
                }

                if (origAllocPtr && origAllocPtrAliasSet.count(cpath)) {
                    std::string stmtReallocPre = getStatementLabel(*dyn_cast<CallInst>(reallocInst), "pre");
                    if (conditionCode == BRANCH_WHERE_VAR_IS_NULL) {
                        outFile << "!good(" << cpath << ", " << stmtReallocPre << ") | good(" << cpath << ", " << toPre << ") # Path where realloc fails and returns NULL\n";
                    } else {
                        outFile << "!good(" << cpath << ", " << stmtReallocPre << ") | zombie(" << cpath << ", " << toPre << ") # Path where realloc succeeds and returns non-NULL\n";
                    }
                    outFile << "!null(" << cpath << ", " << stmtReallocPre << ") | null(" << cpath << ", " << toPre << ")\n";
                    outFile << "!zombie(" << cpath << ", " << stmtReallocPre << ") | zombie(" << cpath << ", " << toPre << ")\n";
                    continue;
                }

                bool is_condPtr_alias = (varComparedToNull != nullptr) && aliases_set.count(parse_cpath(cpath));

                // good(ptr, P-post) -> good(ptr, S-pre)
                // except if P is a conditional jump to S that is conditioned on ptr being null
                if (!(isNullCondJump && is_condPtr_alias && conditionCode == BRANCH_WHERE_VAR_IS_NULL)) {
                    outFile << "!good(" << cpath << ", " << fromPost << ") | good(" << cpath << ", " << toPre << ")\n";
                }
                
                // null(ptr, P-post) -> null(ptr, S-pre)
                // except if P is a conditional jump to S that is conditioned on ptr being non-null
                if (!(isNullCondJump && is_condPtr_alias && conditionCode == BRANCH_WHERE_VAR_IS_NOT_NULL)) {
                    outFile << "!null(" << cpath << ", " << fromPost << ") | null(" << cpath << ", " << toPre << ")\n";
                }
                
                // zombie(ptr, P-post) -> zombie(ptr, S-pre)
                // except if P is a conditional jump to S that is conditioned on ptr being null
                if (!(isNullCondJump && is_condPtr_alias && conditionCode == BRANCH_WHERE_VAR_IS_NULL)) {
                    outFile << "!zombie(" << cpath << ", " << fromPost << ") | zombie(" << cpath << ", " << toPre << ")\n";
                }
            }
        }
    }
    
    void generatePreservationConstraints(const std::string &stmtPre, const std::string &stmtPost, const std::set<std::string> &changedCPaths, const std::set<std::string> &maybeZombifiedCPaths) {
        if (!isDryRun) {
            outFile << "# Preservation of properties during instruction execution\n";
        }
        for (const std::string &cpath : seenCPaths) {
            bool isExcluded = false;
            std::string cur_path = cpath;
            if (maybeZombifiedCPaths.find(cur_path) != maybeZombifiedCPaths.end()) {
                isExcluded = true;
            }
            if (has_suffix(cur_path, "::return")) {
                isExcluded = true;
            }
            bool is_arg_of_called_func = (cur_path.find(std::string("::args::")) != std::string::npos);
            if (is_arg_of_called_func) {
                isExcluded = true;
            }
            if (has_suffix(cur_path, "::return")) {
                isExcluded = true;
            }
            while (true) {
                if (changedCPaths.find(cur_path) != changedCPaths.end()) {
                    isExcluded = true;
                    break;
                }
                if (cur_path[0] == '*') {
                    cur_path = cur_path.substr(1);
                    continue;
                } else {
                    if (!DisableLiveness) {
                        Value* baseVar = cpathToValue[cur_path];
                        if (baseVar != nullptr && cur_path != std::string("NULL_CONST") && currentLiveVars.count(baseVar) == 0) {
                            isExcluded = true;
                        }
                    }
                    break;
                }
            }

            if (!isExcluded) {
                if (!isDryRun) {
                    // good(x, S-post) -> good(x, S-pre)
                    outFile << "!good(" << cpath << ", " << stmtPre << ") | good(" << cpath << ", " << stmtPost << ")\n";
                    
                    // null(x, S-post) -> null(x, S-pre)
                    outFile << "!null(" << cpath << ", " << stmtPre << ") | null(" << cpath << ", " << stmtPost << ")\n";
                    
                    // zombie(x, S-post) -> zombie(x, S-pre)
                    outFile << "!zombie(" << cpath << ", " << stmtPre << ") | zombie(" << cpath << ", " << stmtPost << ")\n";
                }
            }
        }
        if (!isDryRun) {
            outFile << "# End of preservation\n";
        }
    }

    std::string replaceBadCharsInVarName(const std::string& input) {
        std::string result;
        result.reserve(input.size()); // Pre-allocate to avoid reallocations
        
        for (char ch : input) {
            if ((ch >= 'A' && ch <= 'Z') ||
                (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '_' || ch == ':') {
                result += ch;
            } else {
                result += '_';
            }
        }
        
        return result;
    }
    
    std::string replaceBadCharsInFilename(const std::string& input) {
        std::string result;
        result.reserve(input.size()); // Pre-allocate to avoid reallocations
        
        for (char ch : input) {
            if ((ch >= '-' && ch <= 'Z') ||
                (ch >= '_' && ch <= 'z'))
            {
                result += ch;
            } else {
                result += '_';
            }
        }
        
        return result;
    }
    
    std::string getCPath(Value &V) {

        if (isa<Instruction>(&V) && already_renamed_cur_func_values) {
            std::string ret = V.getName().str();
            if (ret.empty()) {
                errs() << "\nError: empty name for: ";
                V.dump();
                abort();
            }
            return ret;
        }

        std::stringstream ss;
        
        // Try to get source variable name from debug info
        std::string sourceName;
        auto it = valueToSourceName.find(&V);
        if (it != valueToSourceName.end()) {
            sourceName = it->second;
        } else if (V.hasName()) {
            sourceName = V.getName().str();
        }

        Instruction* inst = dyn_cast<Instruction>(&V);
        unsigned inst_id = 0;
        if (inst) {
            inst_id = getInstructionId(*inst);
            if (AllocaInst* alloca = dyn_cast<AllocaInst>(inst)) {
                if (!sourceName.empty()) {
                    sourceName = "addr_" + sourceName;
                }
            }
            if (LoadInst* load = dyn_cast<LoadInst>(inst)) {
                if (AllocaInst* alloca = dyn_cast<AllocaInst>(load->getPointerOperand())) {
                    if (sourceName.empty()) {
                        sourceName = alloca->getName().str();
                    }
                }
            }
            std::string origName = get_pom_orig_var_name(*inst);
            if (!origName.empty()) {
                sourceName = origName;
            }
        }
        if (!sourceName.empty()) {
            ss << replaceBadCharsInVarName(sourceName);
        } else if (inst_id) {
            bool got_load_special = false;
            //if (LoadInst* load = dyn_cast<LoadInst>(inst)) {
            //    Value* ptr_value = load->getPointerOperand();
            //    if (isa<Instruction>(ptr_value) && !isa<LoadInst>(ptr_value)) {
            //        std::string ptr_name = getCPath(*ptr_value);
            //        // Remove "_NNN" from the end.
            //        size_t pos_underscore = ptr_name.find_last_of('_');
            //        if (pos_underscore != std::string::npos) {
            //            ptr_name.erase(pos_underscore);
            //        }
            //        ss << "star_" << ptr_name;
            //        got_load_special = true;
            //    }
            //}
            if (!got_load_special) {
                ss << "vI" << inst_id;
            }
        } else {
            ss << "temp";
        }
        
        // Always append unique identifier to ensure uniqueness
        // Check if we've already assigned an ID to this value
        auto varIt = valueToVarId.find(&V);
        if (varIt != valueToVarId.end()) {
            ss << "_" << varIt->second;
        } else {
            // Assign new ID
            unsigned varId = nextVarId++;
            valueToVarId[&V] = varId;
            ss << "_" << varId;
        }
        
        std::string ret;
        if (isa<ConstantPointerNull>(&V)) {
            ret = std::string("NULL_CONST");
        } else if (isa<Argument>(V)) {
            ret = getArgName(dyn_cast<Argument>(&V));
        } else {
            ret = ss.str();
        }
        cpathToValue[ret] = &V;
        return ret;
    }

    std::string getArgName(Argument* arg) {
        //return std::string("arg:") + std::to_string(arg->getArgNo()) + ":" + arg->getName().str();
        std::string prefix = GloUniq ? arg->getParent()->getName().str() + ":" : "";
        return prefix + std::string("arg:") + replaceBadCharsInVarName(arg->getName().str());
    }

    // Function to find a nearby instruction with valid debug location
    Instruction* findNearbyRealInst(Instruction &I) {
        BasicBlock *BB = I.getParent();
        
        // Start from the instruction after the given instruction
        BasicBlock::iterator it = std::next(I.getIterator());
        
        // Walk forward in the basic block
        for (BasicBlock::iterator end = BB->end(); it != end; ++it) {
            Instruction *candidate = &*it;
            
            // Check if this instruction has debug location with non-zero line number
            if (const DebugLoc &DL = candidate->getDebugLoc()) {
                if (DL.getLine() != 0) {
                    return candidate;
                }
            }
        }
        
        // If no instruction with non-zero line number found, return the original instruction
        return &I;
    }

    Instruction* get_first_real_inst_of_BB(BasicBlock &BB) {
        for (Instruction &I : BB) {
            if (isDbgIntrinsic(I)) {
                continue;
            }
            return &I;
        }
        errs() << "Warning: No real instruction found in basic block!\n";
        return nullptr;
    }

    std::string getStatementLabel(Instruction &I, const std::string &suffix) {
        std::stringstream ss; // return value, to be populated.

        // Special Case 1: Entry point with "pre" suffix
        if (suffix == "pre") {
            Function *F = I.getParent()->getParent();
            BasicBlock &entryBB = F->getEntryBlock();
            if (I.getParent() == &entryBB && &I == get_first_real_inst_of_BB(entryBB)) {
                return "start";
            }


        }
        
        // Special Case 2: Return instruction with "post" suffix
        if (suffix == "post" && isa<ReturnInst>(&I)) {
            return "end";
        }
        
        Instruction *instToUse = &I;
        bool useColumnZero = false;
        
        // Check if we should try to find a nearby real instruction
        bool needsRealInst = false;
        if (const DebugLoc &DL = I.getDebugLoc()) {
            // Has debug location but line number is 0
            if (DL.getLine() == 0) {
                needsRealInst = true;
            }
        } else {
            // No debug location at all
            needsRealInst = true;
        }
        
        // If we need to find a real instruction, try to find one
        if (needsRealInst) {
            instToUse = findNearbyRealInst(I);
            useColumnZero = true;
        }
        
        // Try to get debug location from the chosen instruction
        unsigned Line = 0;
        if (const DebugLoc &DL = instToUse->getDebugLoc()) {
            Line = DL.getLine();
            unsigned Col = useColumnZero ? 0 : DL.getCol();
            
            // Include filename if available
            if (DIScope *Scope = llvm::dyn_cast<llvm::DIScope>(DL.getScope())) {
                if (DIFile *File = Scope->getFile()) {
                    std::string filename = File->getFilename().str();
                    // Extract just the basename for readability
                    size_t lastSlash = filename.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        filename = filename.substr(lastSlash + 1);
                    }
                    if (filename.length() > 8+3+6+4) {
                        filename = filename.substr(0, 8) + "..." + filename.substr(filename.length()-6-4);
                    }
                    filename = replaceBadCharsInFilename(filename);
                    ss << filename << "_";
                }
            }
            if (isa<PHINode>(I)) {
                ss << "L" << Line << "_phi";
            } else if (isDbgIntrinsic(I)) {
                ss << "L" << Line << "_dbg";
            } else {
                ss << "L" << Line << "c" << Col;
            }
        } else {
            // Fallback when no debug info available
            ss << "S";
        }
        if (Line == 0) {
            // errs() << "[DBG] Missing debug line for instruction: "
            //        << I.getOpcodeName();
            // errs() << "\n";
            // I.dump();
        }
        
        // Always append unique instruction ID to ensure uniqueness
        ss << "_I" << getInstructionId(I);
        
        if (suffix != "") {
            ss << "-" << suffix;
        }
        return ss.str();
    }

    unsigned getInstructionId(Instruction &I) {
        auto it = instToId.find(&I);
        if (it != instToId.end()) {
            return it->second;
        }
        return 0; // fallback
    }

    bool isDbgIntrinsic(Instruction &I) {
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
            if (Function *F = CI->getCalledFunction()) {
                #if CLANG_VER < 17
                if (F && F->getName().startswith("llvm.dbg.")) {
                    return true;
                }
                #else
                if (F && F->getName().starts_with("llvm.dbg.")) {
                    return true;
                }
                #endif
            }
        }
        return false;
    }
};

// Old Pass Manager Implementation (Clang 15)
#if CLANG_VER < 17
namespace {
    struct ConstraintGenPass : public ModulePass {
        static char ID;
        ConstraintGenImpl impl;
        
        ConstraintGenPass() : ModulePass(ID) {}
        
        bool runOnModule(Module &M) override {
            return impl.runOnModule(M);
        }
    };
}

char ConstraintGenPass::ID = 0;
static RegisterPass<ConstraintGenPass> X("constraint-gen", "Generate pointer constraints", false, false);
#endif



// New Pass Manager Implementation (Clang 18+)
#if CLANG_VER >= 17
namespace {
    struct ConstraintGenPassNewPM : public PassInfoMixin<ConstraintGenPassNewPM> {
        ConstraintGenImpl impl;
        
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            bool Modified = impl.runOnFunction(F);
            return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
        }
    };
}

// Register with new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ConstraintGenPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "constraint-gen") {
                        MPM.addPass(ConstraintGenPassNewPM());
                        return true;
                    }
                    return false;
                });
        }};
}
#endif
