/******************************************************************************
* GhostQ: A QBF solver with ghost variables.
* Version 2017-07-26
* Copyright (C) Will Klieber
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
******************************************************************************/



/* Debugging breakpoint */
static void rawstop() { }
static void (*volatile stop)() = &rawstop;  // For debugging; optimizer won't remove.

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <queue>
using namespace std;

#include <ext/hash_set>
using namespace __gnu_cxx;

#include "parser.hh"
#include "fmla.hh"
#include "quickselect.cpp"

extern void int_tim_sort(int *dst, const size_t size);

typedef unsigned long long uint64;


/*****************************************************************************
* Utility Macros.    }}}{{{1
*****************************************************************************/

#define auto(var, init) typeof(init) var = (init)

#define foreach(var, container) \
  for (auto(var, (container).begin());  var != (container).end();  ++var)

#define printferr(fmt,args...) \
    ({fflush(stdout); fprintf(stderr, fmt, ##args);})

#define talloc(type, n) ((type*) calloc(n, sizeof(type)))

// Unicode character 0x0394 (GREEK CAPITAL LETTER DELTA)
#define UTF8_DELTA "\xCE\x94"




/*****************************************************************************
* Vim: highlight func name in func definition, folding.     }}}{{{1    
******************************************************************************
* syn match cppFuncDef "\~\?\zs\h\w*\ze([^)]*\()\s*\(const\)\?\)\?{ *$"
* hi def link cppFuncDef Todo
* set fdm=marker
*****************************************************************************/


/**************************************************************
* Global constants.    }}}{{{1
**************************************************************/

#define QTYPE_E1 1
#define QTYPE_A0 0
#define QTYPE_BAD 999
#define ZERO_DECLEV 0
#define ZERO_LIT 0


/* Global command-line and compile-time options. */
int ExDbg = false;
bool JustPrint = false;


#ifndef GloDbgLev
int GloDbgLev = 10;
#endif

#ifdef NO_SAN_CHK
#define GloSanChk 0
#else
int GloSanChk = 1;
#endif

int GloRandSeed;
long long GloTimeOut = 0;
long long StartTime;
int RestartCycle = 32;
int NoRestart = false;
int UseMonotone = 0;
int NoUnivGhost = 0;
bool VarOrdFix = false;
int expu = 0;
FILE* PrfLog = NULL;
bool QuietMode = false;

double max_learnts;
double learntsize_factor = 1.0/3.0;
double learntsize_adjust_confl = 100;
double learntsize_adjust_inc = 1.5;
double learntsize_inc = 1.1;
int learntsize_adjust_cnt = (int)(learntsize_adjust_confl);

const int LINE_LEN = 90;



/*****************************************************************************
* Utility Functions.    }}}{{{1
*****************************************************************************/

inline int StrEqc(const char* s1, const char* s2) {
    // Precond: At least one of the strings is not null.
    if ((s1 == NULL) ^ (s2 == NULL)) {return 0;}
    return (strcasecmp(s1, s2) == 0);
}

int die_f(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
    return 0;
}

int die(const char* msg="") {
    fprintf(stderr, "%s", msg);
    abort();
    return 0;
}

long long GetTimeMicro() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((long long) tv.tv_sec) * 1000000 + (tv.tv_usec);
}

long long GetUserTimeMicro() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    struct timeval tv = usage.ru_utime;
    return ((long long) tv.tv_sec) * 1000000 + (tv.tv_usec);
}

int RandSeedFromTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int ret = tv.tv_sec * 10000 + (tv.tv_usec / 100);
    ret = (ret & (((uint)(~0)) >> 1));  // Set the MSB to zero.
    return ret;
}

void DumpIntSet(set<int>* pSet, const char* pEnd) {
    if (pSet == NULL) {printf("(null)%s", pEnd); return;}
    set<int>::iterator iter = pSet->begin();
    printf("[ ");
    for (; iter != pSet->end(); ++iter) {
        printf("%i ", *iter);
    }
    printf("]%s", pEnd);
}

bool gt_f(int const& x, int const& y) {
    return (x > y);
}

bool lt_f(int const& x, int const& y) {
    return (x < y);
}

template <typename K, typename V>
V condget(map<K,V>* pSet, K key, V Default) {
    typename map<K,V>::iterator it = pSet->find(key);
    if (it == pSet->end()) {
        return Default;
    }
    return it->second;
}


/*****************************************************************************
* Fixed-Size Array with Bounds Info.    }}}{{{1
*****************************************************************************/
template <typename T>
struct ap {  // "Array Pointer"
    T* arr;
    int n;

    /* Constructor */
    ap() : arr(NULL), n(0) { }; 
    ap(int* _arr, int _n) : arr(_arr), n(_n) { }; 

    T& operator[](unsigned ix) const {return arr[ix];}
    T& last() {return arr[n-1];}
    void alloc(int size) {
        n = size;
        arr = talloc(T, size);
    }

    T* begin() {return &arr[0];}
    T* end() {return &arr[n];}

    #define FPROTO_ap
    #include "ghostq.proto"
    #undef FPROTO_ap

};

template <typename T>
int ap<T>::indexof(T x) {
    for (int i=0; i < n; i++) {
        if (arr[i] == x) {
            return i;
        }
    }
    return -1;
}

template <typename T>
void ap<T>::remove_adj_dups() {
    T* pNewEnd = unique(begin(), end());
    n = pNewEnd - begin();
}

template <typename T>
int deep_eq(ap<T> a1, ap<T> a2) {
    if (a1.n != a2.n) return false;
    for (int i=0; i < a1.n; i++) {
        if (a1.arr[i] != a2.arr[i]) return false;
    }
    return true;
}

template <typename T>
void ap<T>::dealloc() {
    n = 0;
    free(arr);
    arr = NULL;
}

template <typename T>
ap<T> ap<T>::dup() {
    ap<T> ret = ap<T>();
    ret.n = n;
    ret.arr = talloc(T, n);
    for (int i=0; i < n; i++) {
        ret[i] = arr[i];
    };
    return ret;
}



/*****************************************************************************
* Growable Array.    }}}{{{1
* We use this instead of the STL vector class to make debugging a less horrible
* experience and to manually optimize memory usage.
*****************************************************************************/
template <typename T>
struct garr {
    T* arr;
    int n;
    int MaxSize;

    #define FPROTO_garr
    #include "ghostq.proto"
    #undef FPROTO_garr

    /* Constructor */
    garr() : arr(NULL), n(0), MaxSize(0) { }; 
    garr(int _size) : n(_size), MaxSize(_size) {
        arr = talloc(T, _size);
    }; 
    garr(int _size, int prealloc) : n(_size), MaxSize(prealloc) {
        assert(n <= prealloc);
        arr = talloc(T, prealloc);
    }; 

    void setval(int ix, T val) {
        if (ix >= n) {
            resize_bzero(ix + 1);
        }
        arr[ix] = val;
    }

    T getval(int ix, T default_val) {
        if (ix < n) {
            return arr[ix];
        } else {
            return default_val;
        }
    }
    
    T& operator[](unsigned ix) const {return arr[ix];}

    T& last() const {return arr[n-1];}

    void init(int _size=0, int prealloc=8)  {
        n = _size;
        MaxSize = prealloc = max(prealloc, _size);
        if (MaxSize == 0) {arr = NULL;}
        else {
            arr = talloc(T, prealloc);
            if (arr == NULL) {
                int memsize = prealloc * sizeof(T);
                die_f("\nFatal error: Unable to allocate contiguous chunk of %i bytes of memory!\n", memsize);
            }
        }
    };

    void resize(int NewSize) {
        if (NewSize > MaxSize) {realloc_grow(NewSize);}
        n = NewSize;
    };

    void resize_fill(int NewSize, T pad) {
        if (NewSize > MaxSize) {realloc_grow(NewSize);}
        for (int i = n; i < NewSize; i++) {
            arr[i] = pad;
        }
        n = NewSize;
    };

    void grow_fill(int NewSize, T pad) {
        if (n >= NewSize) return;
        resize_fill(NewSize, pad);
    };

    void resize_bzero(int NewSize) {
        if (NewSize > MaxSize) {
            realloc_grow(NewSize);
        }
        if (NewSize > n) {
            memset(arr + n, 0, (NewSize - n) * sizeof(T));
        }
        n = NewSize;
    };

    T* begin() {return &arr[0];}
    T* end() {return &arr[n];}
    int size() const {return n;}

    bool bin_search(T val) {
        return binary_search(this->begin(), this->end(), val);
    }

    void append(T item) {resize(n+1); arr[n-1] = item;};

    T pop() {
        assert(n > 0);
        n--;
        return arr[n];
    }

    typedef T* iterator;

};


template <typename T>
int garr<T>::realloc_grow(int DemandSize) {
    int NewSize;
    if (MaxSize < 65536) {
        if (MaxSize < 8) {
            MaxSize = 8;
        }
        NewSize = MaxSize * 2;
    } else {
        NewSize = (MaxSize + (MaxSize / 4)) & ~4096;
    }
    if (NewSize < DemandSize) {
        NewSize = DemandSize;
    }
    MaxSize = NewSize;
    arr = (T*) realloc(arr, NewSize*sizeof(T));
    if (arr == 0) throw "Fatal error: alloc failed!";
    return 0;
};

template <typename T>
void garr<T>::realloc_shrink(int NewSize=-1) {
    if (NewSize == -1) {
        NewSize = n;
    }
    MaxSize = NewSize;
    arr = (T*) realloc(arr, NewSize*sizeof(T));
};

template <typename T>
T* garr<T>::AppendBlank() {
    resize(n+1);
    arr[n-1] = T();
    memset(&arr[n-1], 0, sizeof(arr[n-1]));
    return &arr[n-1];
}

template <typename T>
void garr<T>::extend(garr<T> items) {
    int base = n;
    int m = items.n;
    resize(n + m);
    for (int i=0; i < m; i++) {
        arr[base+i] = items[i];
    }
};

template <typename T>
void garr<T>::extend(ap<T> items) {
    int base = n;
    int m = items.n;
    resize(n + m);
    for (int i=0; i < m; i++) {
        arr[base+i] = items[i];
    }
};

template <typename T>
void garr<T>::dealloc() {
    n = MaxSize = 0;
    free(arr);
    arr = NULL;
}

template <typename T>
garr<T> garr<T>::dup() {
    garr<T> ret;
    ret.init(n, n);
    for (int i=0; i < n; i++) {
        ret[i] = arr[i];
    };
    return ret;
}

template <typename T>
int garr<T>::indexof(T x, int start=0) {
    for (int i=start; i < n; i++) {
        if (arr[i] == x) {
            return i;
        }
    }
    return -1;
}

template <typename T>
T garr<T>::RemoveAndFill(int ix) {
    T ret = arr[ix];
    arr[ix] = arr[n-1];
    n--;
    return ret;
}

template <typename T>
T garr<T>::PopLast() {
    assert(n > 0);
    T ret = arr[n-1];
    n--;
    return ret;
}

template <typename T>
void garr<T>::RemoveVal_AndFill(T val) {
    for (int i=0; i < n; i++) {
        if (arr[i] == val) {
            RemoveAndFill(i);
            return;
        }
    }
    assert(false);
}

template <typename T>
garr<T> garr<T>::dup_shuffle() {
    garr<T> tmp = this->dup();
    garr<T> ret;
    ret.init(n);
    for (int i=0; i < n; i++) {
        ret[i] = tmp.RemoveAndFill(rand() % (n-i));
    }
    tmp.dealloc();
    return ret;
}

template <typename T>
garr<T> garr<T>::dup_apply(T (*fn)(T)) {
    garr<T> ret;
    ret.init(n, n);
    for (int i=0; i < n; i++) {
        ret[i] = fn(arr[i]);
    };
    return ret;
}

template <typename T>
void garr<T>::apply(T (*fn)(T)) {
    for (int i=0; i < n; i++) {
        arr[i] = fn(arr[i]);
    };
}

template <typename T>
void garr<T>::stl_sort() {
    sort(this->begin(), this->end());
}

template <typename T>
void garr<T>::remove_adj_dups() {
    T* pNewEnd = unique(begin(), end());
    n = pNewEnd - begin();
}

template <typename T>
void garr<T>::remove_value(int badval) {
    T* pNewEnd = std::remove(begin(), end(), badval);
    n = pNewEnd - begin();
}
template <typename T>
void ap<T>::remove_value(int badval) {
    T* pNewEnd = std::remove(begin(), end(), badval);
    n = pNewEnd - begin();
}


template <typename T, typename C>
void init_garr_from_set(garr<T>* pGarr, set<T,C>& OrigSet) {
    pGarr->resize(OrigSet.size());
    pGarr->n = 0;
    typename set<T>::iterator iter = OrigSet.begin();
    for (; iter != OrigSet.end(); ++iter) {
        pGarr->append(*iter);
    }
}

template <typename T, typename C>
void init_garr_from_list(garr<T>* pGarr, list<T,C>& OrigSet) {
    pGarr->resize(OrigSet.size());
    pGarr->n = 0;
    typename list<T>::iterator iter = OrigSet.begin();
    for (; iter != OrigSet.end(); ++iter) {
        pGarr->append(*iter);
    }
}

template <typename T>
T garr<T>::find_median_destructively() {
    T ret = quick_select(this->arr, this->n);
    return ret;
}

typedef garr<int> garri;

template <typename T>
garr<T> dup_as_garr(ap<T>& cur) {
    garr<T> ret;
    ret.init(cur.n, cur.n);
    for (int i=0; i < cur.n; i++) {
        ret[i] = cur.arr[i];
    };
    return ret;
}

template <typename T>
ap<T> dup_as_ap(garr<T>& cur) {
    ap<T> ret;
    ret.arr = talloc(T, cur.n);
    ret.n = cur.n;
    for (int i=0; i < cur.n; i++) {
        ret[i] = cur.arr[i];
    };
    return ret;
}


void timsort(garri arr) {
    int_tim_sort(arr.arr, arr.n);
}

void timsort(ap<int> arr) {
    int_tim_sort(arr.arr, arr.n);
}

typedef vector<pair<int, Fmla*> > FullStratT;


/*****************************************************************************
* BitSet.    }}}{{{1
* Unlike the STL bitset, this doesn't require that the number
* of bits be a compile-time constant.
* TODO: Optimize this so that each bit only consumes 1 bit of memory. 
*****************************************************************************/
struct BitSetT {
    garr<char> bmap;

    void init(int n) {bmap = garr<char>(n);}
    void dealloc() {bmap.dealloc();}
    char operator[](uint ix) {return bmap[ix];}
    void set(uint ix, bool val) {bmap[ix] = val;}
    int  count(int ix) {return bmap[ix];}
    void insert(int ix) {bmap[ix] = 1;}
    void erase(int ix) {bmap[ix] = 0;}

    void erase_vals(garri vals) {
        foreach (itVal, vals) {
            erase(*itVal);
        }
    }
    //void clear(int n) {memset(bmap, 0, n);}

    #define FPROTO_BitSetT
    #include "ghostq.proto"
    #undef FPROTO_BitSetT

};



/*****************************************************************************
* Encoding of Literals.    }}}{{{1
*****************************************************************************/

/* Literals are encoded as numbers.  The least-significant bit (LSB)
 * is 0 if the literal is in its positive form, and
 * is 1 if the literal is negated.
 * Currently, we require that the input file uses only even numbers;
 * this makes debugging easier.
 */

typedef int LitT;

LitT encv(int x) {
    if (x >= 0) {return x;}
    else {return (abs(x) + 1);}
}
int decv(LitT x) {
    if ((x & 1) == 1) {return -(x & ~1);}
    else {return x;}
}
inline LitT negv(LitT x) {return (x ^ 1);}
inline LitT absv(LitT x) {return (x & ~1);}
inline LitT signv(LitT x) {return (x & 1);}
inline LitT IsNegLit(LitT x) {return (x & 1);}
inline LitT IsPosLit(LitT x) {return !IsNegLit(x);}


int DumpLitRaw(LitT lit);

template <typename T>
void DumpLitsEnc(T& arr, const char* newl="\n") {
    const char* sep = "";
    printf("[");
    int n = 0;
    foreach (it, arr) {
        n += printf("%s", sep) + DumpLitRaw(*it);  //printf("%s%i", sep, decv(*it));
        if (n > 85) {
            if (n < 95) printf("\n");
            n = 0;
        }
        sep = " ";
    }
    printf("]%s", newl);
}


void DumpLitsEnc(garri& arr, const char* newl="\n") {
    const char* sep = "";
    printf("[");
    int n = 0;
    foreach (it, arr) {
        n += printf("%s", sep) + DumpLitRaw(*it);  //printf("%s%i", sep, decv(*it));
        if (n > 85) {
            if (n < 95) printf("\n");
            n = 0;
        }
        sep = " ";
    }
    printf("]%s", newl);
}




/*****************************************************************************
* Decision Level stuff, and other stuff.    }}}{{{1
*****************************************************************************/
/* Note: QbFromLit[x] is 16 bits long. */
#define MAX_QB ((1 << 15) - 2)

#define NO_DECLEV ((uint)(~0) >> 1)

typedef Fmla* FmlaT;
#define FmlaTrue fmla_true
#define FmlaFalse fmla_false
Fmla NOT_DONE_FMLA;
FmlaT NOT_DONE = (Fmla*) &NOT_DONE_FMLA;

typedef map<int, garr<struct GsT*> >  QbDepListT;  // Maps quantifier index to list of dependent game states. 
typedef QbDepListT::iterator QbDepListIterT;


#define OR_GATE 0x77777777

typedef map<LitT, vector<LitT> > SubstCircuitDefn;


/*****************************************************************************
* Headers.    }}}{{{1
*****************************************************************************/
struct QuantBlkT {
    int     qtype;      // 'A' or 'E' or 'F'.
    int     ix;         // index number
    int     bat;        // gate of which this is a prefix
    garri   lits;
    QuantBlkT() : qtype(0), lits() { }; 
};

struct QuantPfxT {
    garr<QuantBlkT> blks;
};

struct ExprT {
    const char* sOp;
    garri args;
    struct GsT* pLongLs;
};

struct GateDefnT {
    FmlaOp::op op; 
    garri args;
};


/*****************************************************************************
* ProbInstT Header.    }}}{{{1
*****************************************************************************/
struct ProbInst {
    int LastInpLit;
    int RawLastInpLit;
    int FirstGateVar;
    int LastOrigGlit;
    int NextNewVar;
    int NoMoreVars;
    int LastGlit;
    int LastPlit;   // This should now always be equal to LastGlit.
    int OutGlit;
    garr<QuantBlkT> QuantBlk;   // indexed by qb index number
    garr<GsT*> *LitHave;   
    garr<GsT*> clauses;
    garr<FullStratT*> clause_id_to_strat;
    int NumDoneClauses;
    garr<struct GsT*> PhantLitSets;
    int PreprocTimeMilli;

    #define FPROTO_ProbInst
    #include "ghostq.proto"
    #undef FPROTO_ProbInst

    int IsValidInpVar(int x) {return (2 <= x  &&  x <= LastInpLit);}
    int IsValidExprIx(int x) {return (2 <= x  &&  x <= LastGlit);}
    int IsValidQbIx(int x)   {return (1 <= x  &&  x < QuantBlk.n);}

};

ProbInst GloProb = ProbInst();
#define pGloProb (&GloProb)

struct LvCacheEntry {
    int IsAlive;
    long long timestamp;
};


/*****************************************************************************
* Global vars and utility funcs.    }}}{{{1
*****************************************************************************/

/* Global Variables. */
int GloStep = 1;
int* pNumClauses;
bool NeedNewl = false;
int NumImpBlocked = 0;
int NumExecAttempts = 0;
long long NumProps = 0;
long long NumWatchFixes = 0;
int NumReducs = 0;
long long NumStepBt = 1;
int NumBigBt = 0;
long long NumDecs = 0;

int LastInOrderDL = 0;

int LastPickGate = 0;

int NumResolves;

int InnermostInputQb = 0;
int ExtraQb[2] = {0,0};


#define INPUT_LIT ((GsT*) (-1))
#define NON_INPUT ((GsT*) 1)
int LastPhant[2];
int PhantOffset[2];
short *QbFromLit;          // maps a lit to the index of its quant block.
garr<garri> GateDefn;
int *QbToBat;
GsT** GlitToGs;
garr<GsT*> GhostToGs;
GsT** GlitToDerivGs;
QuantPfxT QuantPfx;
short* ReEvalArr;
garr<GateDefnT> GateDefnB;


int LastChoice;
int NumCegLrn = 0;
int AllowCegar = 0;
int AllowFree = 0;
FILE* StratFile = NULL;
bool use_raw_strat = false;
int AllocCegarVars = 0;
int LastOrigClauseID = 0;
unsigned int CurPathMask = 1;
int CurPathNum = 0;
bool print_prf_log_free = false;
#define PathMaskFromNum(i) (1 << (i))
#define PathNum1 1
#define MaxPathNum 1
#define MaxPathMask PathMaskFromNum(MaxPathNum)

double cla_inc = 1;
double var_inc = 1;
set<GsT*> TempLearntClauses = set<GsT*>();
int NumLearntLocked = 0;

int MinPropQb = 0;

uint* LitToDL;       // maps lit to the decision level in which it was chosen or forced
garr<double> LitHit;
double LitHitInc = 1;

Fmla* OrigFmla;
garr<Fmla*> raw_gate_to_fmla;

garr<char*> int_to_str;
garr<Fmla*> raw_int_to_fmla_lit;
map<int, const char*> VarNames;

void populate_int_to_fmla_lit(LitT v) {
    const char* name;
    if (VarNames.count(decv(v))) {
        name = VarNames[decv(v)];
    } else {
        int ok = asprintf(&int_to_str[v], "%i", v);
        if (!ok) {die("Error: asprintf failed!");}
        name = int_to_str[v];
    }
    Fmla* fmla = FmlaVar(name);
    raw_int_to_fmla_lit[v] = fmla;
    raw_int_to_fmla_lit[negv(v)] = fmla->negate();
}

Fmla* int_to_fmla_lit(LitT lit) {
    Fmla* ret = raw_int_to_fmla_lit[lit];
    if (ret != NULL) {
        return ret;
    } else {
        populate_int_to_fmla_lit(absv(lit));
        ret = raw_int_to_fmla_lit[lit];
        return ret;
    }
}

#define USE_PROP_SET 1
#define NO_UNIV_REDUC 0

//hash_set<LitT> WaitingPropSet;
BitSetT WaitingPropSet;
garri WaitingPropLits;


GsT* AbsGlitToGs(int glit) {
    assert(GlitToGs[glit]==0 || GlitToGs[negv(glit)]==0);
    return (GsT*)((long)(GlitToGs[glit]) | (long)(GlitToGs[negv(glit)]));
}

GsT* AbsGlitToDerivGs(int glit) {
    assert(GlitToDerivGs[glit]==0 || GlitToDerivGs[negv(glit)]==0);
    return (GsT*)((long)(GlitToDerivGs[glit]) | (long)(GlitToDerivGs[negv(glit)]));
}

GsT* NegGlitToGs(int glit) {
    return GlitToGs[negv(glit)];
}

void FixNeedNewl() {
    if (NeedNewl) {printf("\n"); NeedNewl = false;}
}

int IsAndGate(int lit) {
    return ((long) GhostToGs[lit]) > 1;
}

inline int IsOrGate(int lit) {
    int ret = (GhostToGs[lit] == NON_INPUT);
    if (ret) {
        assert(IsAndGate(negv(lit)));
    }
    return ret;
}

int GloQtype(int ixQb) {
    return pGloProb->QuantBlk[ixQb].qtype;
}

int QtypeFromLit(int lit) {
    return GloQtype(QbFromLit[lit]);
}

int QtypeNumFromLit(int lit) {
    switch (QtypeFromLit(lit)) {
        case 'A':  return 0;
        case 'E':  return 1;
        default:   assert(false);
    }
}

FmlaT PlyFmlaFromLit(int lit) {
    switch (QtypeFromLit(lit)) {
        case 'A':  return FmlaFalse;
        case 'E':  return FmlaTrue;
        default:   assert(false);
    }
}

int QtypeNumFromLetr(char letr) {
    switch (toupper(letr)) {
        case 'A':  return 0;
        case 'E':  return 1;
        default:   assert(false);
    }
}

int QtypeLetrFromNum(int num) {
    switch (num) {
        case 0:  return 'A';
        case 1:  return 'E';
        default:   assert(false);
    }
}


struct LessGsById {
    bool operator() (const GsT* const& pLs1, const GsT* const& pLs2) const;
};

int NumWatchCleanups = 0;


#define USE_WATCH_SET 0
#define USE_WATCH_LIST 0
#define USE_WATCH_ARR 1

#if USE_WATCH_SET
    typedef set<pair<int, struct GsT*> > GwSetT;
    #define GwSetT_erase_iterator GwSetT::iterator
#elif USE_WATCH_ARR
    #define GwSetT_erase_iterator int
#elif USE_WATCH_LIST
    typedef list<struct GsT*> GwSetT;
    #define GwSetT_erase_iterator GwSetT::iterator
#endif



#define LRN_TYPE_1 1
#define LRN_TYPE_2 2
#define LRN_TYPE_3 3
#define LRN_TYPE_4 4


/*****************************************************************************
* Header for Game-State Entry data structure.    }}}{{{1
*****************************************************************************/
struct GsT {
    int id;
    FmlaT FreeFmla;
    char LrnType;
    char inactive;
    char IsLearned;
    int GlitDefd;

    int NumFix;
    int WatStartPos;
    int WatStopBt;
    int LastUse;
    unsigned int InUse;
    double activity;
    int NumFixes;
    int WatchedRes;
    int WatchRawAE[2];
    #define WatchRawArr WatchRawAE
    GwSetT_erase_iterator itWatchRes;
    GwSetT_erase_iterator itWatchRawAE[2];

    ap<int> ReqLits;    // Required Lits (corresponding to $L^{now}$ in the paper).
    ap<int> WatOrd;     // Watch Order.
    ap<int> ImpLits;    // Reserved Lits (corresponding to $L^{fut}$ in the paper).

    int& WatchLosrAE(int x) {
        return WatchRawAE[x];
    }

    double get_adj_act() {
        return activity;
        // return activity / (NumFixes + 1);
    }

    #define FPROTO_GsT
    #include "ghostq.proto"
    #undef FPROTO_GsT

};


void GsT::init() {
    WatStartPos = 0;
}

vector<LitT>* GsT::AsLitVec() {
    GsT* pCurLs = this;
    vector<LitT>* pRawInts;
    pRawInts = new vector<LitT>(pCurLs->ReqLits.begin(), pCurLs->ReqLits.end());
    foreach (itImp, pCurLs->ImpLits) {
        pRawInts->push_back(*itImp);
    }
    return pRawInts;
}

#if USE_WATCH_SET == 1
int set_count(GwSetT& s, GsT* item) {return s.count(pair<int, struct GsT*>(item->id, item));}  // for debugging
#endif

bool LessGsById::operator() (const GsT* const& pLs1, const GsT* const& pLs2) const {
    //assert(pLs1->id != 0 && pLs2->id != 0);
    if (pLs1->id == pLs2->id) {return pLs1 < pLs2;}
    return (pLs1->id < pLs2->id);
}

bool FnGreaterGsById(const GsT* const& pLs1, const GsT* const& pLs2) {
    return (pLs1->id > pLs2->id);
}

bool FnLessGsById(const GsT* const& pLs1, const GsT* const& pLs2) {
    return (pLs1->id < pLs2->id);
}

int CmpGsById(GsT* const* p1, GsT* const* p2) {
    return ((*p1)->id - (*p2)->id);
}


int CmpLsByPtr(GsT* const* ppLs1, GsT* const* ppLs2) {
    GsT* pLs1 = *ppLs1;
    GsT* pLs2 = *ppLs2;
    return (pLs1 - pLs2);
}




inline GwSetT_erase_iterator* get_gst_itwatch(GsT* pCurLs, LitT wlit) {
    GwSetT_erase_iterator* ret = NULL;
    if (pCurLs->WatchRawAE[0] == wlit) {return &pCurLs->itWatchRawAE[0];}
    if (pCurLs->WatchRawAE[1] == wlit) {return &pCurLs->itWatchRawAE[1];}
    if (pCurLs->WatchedRes    == wlit) {return &pCurLs->itWatchRes;}
    return ret;
}

inline void update_gst_itwatch(GsT* pCurLs, LitT wlit, GwSetT_erase_iterator it) {
    if      (pCurLs->WatchRawAE[0] == wlit) {pCurLs->itWatchRawAE[0] = it;}
    else if (pCurLs->WatchRawAE[1] == wlit) {pCurLs->itWatchRawAE[1] = it;}
    else if (pCurLs->WatchedRes    == wlit) {pCurLs->itWatchRes = it;}
}


#if USE_WATCH_ARR
    class GwSetT {
        public:
        garr<GsT*> arr;
        int num_blanks;
        int wlit;

        /* Constructor */
        GwSetT() {
            arr.init();
            num_blanks = 0;
        }

        typedef GsT** iterator;
        typedef GwSetT_erase_iterator erase_iterator;

        erase_iterator insert(GsT* pCurLs) {
            int num_good = arr.n - num_blanks;
            if (num_blanks > 0 && (num_good == 0 || num_blanks > 4 + num_good)) {
                remove_blanks();
            }
            arr.append(pCurLs);
            erase_iterator ret = arr.n - 1;
            return ret;
        }

        void erase(erase_iterator it, GsT* pCurLs) {
            int pos = it;
            assert (arr[pos] == pCurLs);
            arr[pos] = NULL;
            num_blanks++;
            while (arr.n != 0 && arr.last() == NULL) {
                arr.n--;
                num_blanks--;
            }
        }
        void remove_blanks() {
            NumWatchCleanups++;
            auto(src,  arr.begin());
            auto(dest, arr.begin());
            auto(endptr,  arr.end());
            int NumHits = 0;
            while (src != endptr) {
                auto(pCurLs, *src);
                if (pCurLs != NULL) {
                    if (src != dest) {
                        *dest = *src;
                        update_gst_itwatch(pCurLs, wlit, dest - arr.begin());
                    }
                    dest++;
                } else {
                    NumHits++;
                }
                src++;
            }
            arr.n = dest - arr.begin();
            assert(NumHits == num_blanks);
            num_blanks = 0;
        }

        iterator begin() {return arr.begin();}
        iterator end() {return arr.end();}
    };
#endif



/*****************************************************************************
* ChlitListT: List of chosen (non-forced) literals.    }}}{{{1
*****************************************************************************/
class ChlitListT {
    public:
    garri flat;
    #define FPROTO_ChlitListT
    #include "ghostq.proto"
    #undef FPROTO_ChlitListT
};

void ChlitListT::append(int lit) {
    flat.append(lit);
}

int ChlitListT::pop() {
    return flat.PopLast();
}

int ChlitListT::last() {
    return flat.last();
}

void ChlitListT::dump() {
    int CurQb = 0;
    printf("[");
    for (int i=1; i < flat.n; i++) {
        int lit = flat[i];
        if (i != 1) {printf("%c ", (QbFromLit[lit]==CurQb ? ',' : ';'));}
        CurQb = QbFromLit[lit];
        printf("%i", decv(lit));
    }
    printf("]\n");
}


/*****************************************************************************
* Stuff  }}}{{{1
*****************************************************************************/

struct AntecedT {
    GsT* pDepGs;

    /* Constructor */
    AntecedT() : pDepGs(0) {};
    AntecedT(GsT* _gs) : pDepGs(_gs) {};

    bool operator==(AntecedT x) {
        return (this->pDepGs == x.pDepGs);
    }
};

AntecedT EmptyAnteced;


bool AntecedByGreaterGsId(AntecedT const& a1, AntecedT const& a2) {
    return (a1.pDepGs->id < a2.pDepGs->id);
}


/****************************************************************************/
struct GsPriQ {
    garr<AntecedT> a;
    int first;

    void init() {
        a.init();
    }
    void append(AntecedT ante) {
        a.append(ante); 
        push_heap(a.begin(), a.end(), AntecedByGreaterGsId);
    }
    AntecedT pop()  {
        if (a.n == 0) return EmptyAnteced;
        pop_heap(a.begin(), a.end(), AntecedByGreaterGsId);
        AntecedT ret = a.PopLast();
        return ret;
    }
    AntecedT peek() {return (a.n == 0 ? EmptyAnteced : a[0]);}
    bool is_empty() {return (a.n == 0);}

    void clear() {
        a.n=0; 
    }
};



template <typename T, bool (*lt)(T const& x, T const& y)>
struct heap {
    garr<T> h;

    void insert(T x) {
        h.append(x);
        push_heap(h.begin(), h.end(), lt);
    }

    T pop() {
        assert(h.n > 0);
        T ret = h[0];
        while (h.n > 0 && h[0] == ret) {
            pop_heap(h.begin(), h.end(), lt);
            h.n--;
        }
        return ret;
    }

    T peek() {
        return h[0];
    }

    bool empty() {return (h.n == 0);}

    T top() {return h[0];}
    void push(T x) {insert(x);}
    
};



class GsBuilder;


#define CONFLICT_GST_PRI 1
#define FORC_GST_PRI 2
#define MAX_GST_PRI 3

   
#define EM_DRYRUN 0
#define EM_SCHED 1
#define EM_WIN 3


typedef int ChronoT;

/*****************************************************************************
* AsgnT Header.    }}}{{{1
*****************************************************************************/
struct AsgnT {
    ChlitListT chlits;   // Chosen lits, arranged according to quantifier block
    GwSetT *LitWatch;    // LitWatch[lit] lists the game-states that are watching lit.
    garri* UndoListByDL;
    AntecedT* DepByLit;
    ChronoT* LitToChrono;
    garr<char> PolPref[MaxPathNum+1];  // Polarity Preference
    LitT* ChronoToLit;
    FmlaT FreeFmla;
    set<LitT> LitsInConflict;
    GsT* pConflictGs;


    int CurExpSplit;
    int NumNewNodes;

    int CurChrono;
    int LastInitGs;
    int LastDefnGs;
    int DeathChrono;
    long long NumAppends; // For debugging.

    GsPriQ PendingExecs[MAX_GST_PRI+1]; 

    garri InitChoices;
    int ixChoi;
    int NumSinceCut;
    int NumUntilRestart;
    long long LastPrintTime;
    int NumRestarts;
    set<pair<int, vector<int> > > runs;

    //long long* LitToAbsTime;  // For debugging only
    //int *LastHitDL;           // For debugging only



    #define FPROTO_AsgnT
    #include "ghostq.proto"
    #undef FPROTO_AsgnT
    
    inline bool HasLit(int x) {return LitToDL[x] != NO_DECLEV;}
    inline bool HasLitOrNeg(int x) {return HasLit(x) || HasLit(negv(x));}

    inline int GetLastChlit() {
        return chlits.last();
    }

    inline uint GetCurDL() {
        return LitToDL[GetLastChlit()];
    }

    bool IsChlit(int lit) {
        return (ChlitByDL(LitToDL[lit]) == lit);
    }

    inline GwSetT_erase_iterator LitWatchInsert(int lit, GsT* pGs) {
        #if USE_WATCH_SET
        return LitWatch[lit].insert(pair<int,GsT*>(pGs->id, pGs)).first;
        #elif USE_WATCH_ARR
        return LitWatch[lit].insert(pGs);
        #elif USE_WATCH_LIST
        return LitWatch[lit].insert(LitWatch[lit].end(), pGs);
        #endif
    }
    inline void LitWatchErase(int lit, GsT* pGs, GwSetT_erase_iterator it) {
        #if USE_WATCH_ARR
        LitWatch[lit].erase(it, pGs);
        #else
        LitWatch[lit].erase(it);
        #endif
    }
};

/****************************************************************************/

AsgnT GloAsgn = AsgnT();
#define pGloAsgn (&GloAsgn)

/****************************************************************************/
#include "Heap.h"

garr<double> VarActivity;

struct VarActLt {
    bool operator () (int x, int y) const {
        if (VarActivity[x] == VarActivity[y]) {
            return x < y;
        }
        return VarActivity[x] > VarActivity[y];
    }
};

VarActLt VarActLtInst;

template <class Lt>
class VarOrderByQb {
    public:
    garr<Heap<Lt> > order_heap;
    Lt comp;

    VarOrderByQb(Lt c) {
        comp = c;
        order_heap = garr<Heap<Lt> >();
    }

    void add_qb(int max_qb) {
        while (order_heap.n <= max_qb) {
            order_heap.append(Heap<Lt>(comp));
        }
    }

    void insert(int var) {
        assert(absv(var) == var);
        int qb = QbFromLit[var];
        if (!order_heap[qb].inHeap(var)) {
            order_heap[qb].insert(var);
        }
    }

    bool inHeap(int var) {
        int qb = QbFromLit[var];
        return order_heap[qb].inHeap(var);
    }

    LitT removeMin() {
        for (int qb=0; qb < order_heap.size(); qb++) {
            if (order_heap[qb].size() > 0) {
                return order_heap[qb].removeMin();
            }
        }
        return 0;
    }

    void decrease_key(int var) {
        int qb = QbFromLit[var];
        order_heap[qb].decrease(var);
    }
};

VarOrderByQb<VarActLt> VarActOrder = VarOrderByQb<VarActLt>(VarActLt());




GsT* AsgnT::GetDepByLit(int lit) {
    return DepByLit[lit].pDepGs;
}

void AsgnT::SetDepByLit(int lit, GsT* pDepGs) {
    assert((pDepGs->InUse & CurPathMask) == 0);
    pDepGs->InUse |= CurPathMask;
    if (pDepGs->IsLearned) {
        NumLearntLocked++;
    }
    DepByLit[lit].pDepGs = pDepGs;
}

void AsgnT::ResetDepByLit(int lit, bool KeepDepLocked) {
    GsT* pDepGs = DepByLit[lit].pDepGs;
    if (!pDepGs) return;
    assert((pDepGs->InUse & CurPathMask));
    if (!KeepDepLocked) {
        pDepGs->InUse &= ~CurPathMask;
        if (pDepGs->IsLearned) {
            NumLearntLocked--;
        }
    }
    DepByLit[lit].pDepGs = NULL;
}

int PlyFromLit(int lit) {
    return QtypeNumFromLit(lit);
}

int frob_raw_gate(int glit) {
    int pol = signv(glit);
    int ret = (absv(glit)*2) ^ pol;
    return ret;
}

int ghost_to_orig(int glit) {
    int pol = signv(glit);
    int ret = absv(absv(glit)/2) ^ pol;
    return ret;
}

int ghosted(int glit, int ply) {
    int pol = signv(glit);
    int gvar = absv(glit);
    assert((gvar & 0x2) == 0);
    int ret = (gvar | (ply*2)) ^ pol;
    assert(pGloProb->RawLastInpLit < ret);
    return ret;
}

extern int ga(int glit) {return ghosted(frob_raw_gate(glit), 0);}
extern int ge(int glit) {return ghosted(frob_raw_gate(glit), 1);}

bool is_ghost(int lit) {
    int ret = (GhostToGs[lit] != INPUT_LIT);
    //assert(ret == (pGloProb->RawLastInpLit < lit));
    return ret;
}

int ghosted_or_pass(int lit, int ply) {
    if (is_ghost(lit)) {
        return ghosted(lit, ply);
    } else {
        return lit;
    }
}

bool IsInputLit(int glit) {
    return (GhostToGs[glit] == INPUT_LIT);
}

inline bool IsOrigLit(int lit) {
    return (lit <= pGloProb->LastOrigGlit);
}

bool IsReallyGateLit(int glit) {
    return (GhostToGs[glit] != INPUT_LIT);
}

int PrintLitRaw(LitT lit, FILE* out) {
    if (IsInputLit(lit)) {
        return fprintf(out, "%s%s", 
            IsNegLit(lit) ? "-" : "", 
            (char*) int_to_fmla_lit(absv(lit))->arg[0]);
    } else {
        bool IsExist = ((lit & 2) != 0);
        return fprintf(out, "%i%s", decv(ghost_to_orig(lit)), IsExist ? "ge" : "ga");
    }
}
int DumpLitRaw(LitT lit) {
    return PrintLitRaw(lit, stdout);
}

FmlaT ghost_var_to_orig_fmla(int ghost_lit) {
    int glit = ghost_to_orig(ghost_lit);
    bool is_neg = IsNegLit(glit);
    FmlaT ret = raw_gate_to_fmla.getval(absv(glit), NULL);
    assert(ret != NULL);
    if (is_neg) {
        ret = ret->negate();
    }
    return ret;
}


/* For debugging */
void DumpLit(int lit) {
    DumpLitRaw(lit);
    printf("\n");
}
    

// Prototype declaration
void add_gate_to_qb(int ixExpr, garri args);
void add_var_to_qblock(int qvar, int ixQb);



/* Strategy generation */

FullStratT* merge_strats(FullStratT* strat1, FullStratT* strat2, int resolvent) {
    auto(itS1, strat1->begin());
    auto(itS2, strat2->begin());
    auto(endS1, strat1->end());
    auto(endS2, strat2->end());
    FullStratT* ret = new FullStratT();
    if (itS1 != endS1 || itS2 != endS2) {
        stop();
    }
    const int NO_VAR = 1 << 30;
    while (true) {
        int s1_var = NO_VAR;
        int s2_var = NO_VAR;
        if (itS1 != endS1) {s1_var = itS1->first;}
        if (itS2 != endS2) {s2_var = itS2->first;}
        if (s1_var == NO_VAR && s2_var == NO_VAR) {
            break;
        }
        if (s1_var < s2_var) {
            ret->push_back(*itS1);
            itS1++;
        } else if (s2_var < s1_var) {
            ret->push_back(*itS2);
            itS2++;
        } else {
            assert(s1_var == s2_var);
            Fmla* s1_fmla = itS1->second;
            Fmla* s2_fmla = itS2->second;
            Fmla* resolvent_fmla;
            if (is_ghost(resolvent)) {
                resolvent_fmla = ghost_var_to_orig_fmla(resolvent);
            } else {
                resolvent_fmla = int_to_fmla_lit(resolvent);
            }
            assert(resolvent_fmla != NULL);
            Fmla* merged_fmla = ConsFmla(FmlaOp::ITE, 
                resolvent_fmla, s1_fmla, s2_fmla);
            ret->push_back(pair<int,Fmla*>(s1_var, merged_fmla));
            itS1++;
            itS2++;
        }
    }
    return ret;
}

FullStratT* strat_from_Lfut(ap<int> Lfut) {
    FullStratT* ret = new FullStratT();
    foreach (itLit, Lfut) {
        int lit = *itLit;
        ret->push_back(pair<int,Fmla*>(absv(lit), IsPosLit(lit) ? fmla_true : fmla_false));
    }
    return ret;
}

Fmla* strat_to_list_fmla(FullStratT* strat) {
    map<int, vector<pair<int,Fmla*> > > vars_by_qb = map<int, vector<pair<int,Fmla*> > >();
    foreach (it, *strat) {
        int var = it->first;
        vars_by_qb[QbFromLit[var]].push_back(pair<int,Fmla*>(var, it->second));
    }
    map<Fmla*,Fmla*> submap = map<Fmla*,Fmla*>();
    vector<Fmla*> ret_vec = vector<Fmla*>();
    ret_vec.reserve(strat->size());
    foreach (itQb, vars_by_qb) {
        //printf("QB %i: ", itQb->first);
        map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
        foreach (itVar, itQb->second) {
            Fmla* fmla_var = raw_int_to_fmla_lit[itVar->first];
            Fmla* var_strat = itVar->second;
            if (!use_raw_strat) {
                var_strat = var_strat->subst(submap, cache);
            }
            submap[fmla_var] = var_strat;
            //fmla_var->write(stdout);
            //printf(" ");
            ret_vec.push_back(ConsFmla(FmlaOp::LIST, 
                fmla_var, var_strat));
        }
        //printf("\n");
    }
    return ConsFmla(FmlaOp::LIST, ret_vec);
}

void dump_strat(FullStratT* strat) {
    dump_fmla(strat_to_list_fmla(strat));
}




/* DEBUGGING ****************************************************************/

void VarDL(int var) {
    if (var < 0) {printf("Error.\n"); return;}
    if (var > pGloProb->LastPlit) {printf("Error.\n"); return;}
    int pos = LitToDL[var];
    int neg = LitToDL[var ^ 1];
    if (pos < (1 << 30)) {printf("Pos at DL %u.\n", pos);}
    else if (neg < (1 << 30)) {printf("Neg at DL %u.\n", neg);}
    else {printf("Unassigned.\n");}
}


/****************************************************************************/


int CmpLitByQb(const int *pLit1, const int *pLit2) {
    int q1 = QbFromLit[*pLit1];
    int q2 = QbFromLit[*pLit2];
    if (q1 == q2) {return 0;}
    else if (q1 < q2) {return -1;}
    else {return 1;}
}
        
int CmpLitByDL(const int *pLit1, const int *pLit2) {
    int d1 = LitToDL[*pLit1];
    if (d1==NO_DECLEV &&  LitToDL[negv(*pLit1)]==NO_DECLEV) {d1--;}
    int d2 = LitToDL[*pLit2];
    if (d2==NO_DECLEV &&  LitToDL[negv(*pLit2)]==NO_DECLEV) {d2--;}
    if (d1 == d2) {return 0;}
    else if (d1 < d2) {return -1;}
    else {return 1;}
}

int CmpLitByChrono(const int *pLit1, const int *pLit2) {
    ChronoT d1 = pGloAsgn->LitToChrono[*pLit1];
    ChronoT d2 = pGloAsgn->LitToChrono[*pLit2];
    //return d1 - d2; // NO!
    if (d1 == d2) {return (*pLit1 - *pLit2);}
    else if (d1 < d2) {return -1;}
    else {return 1;}
}

struct LitLessByDL {
    bool operator() (const int& Lit1, const int& Lit2) const {
        int d1 = LitToDL[Lit1];
        int d2 = LitToDL[Lit2];
        if (d1 == d2) {
            if (Lit1 == Lit2) return false;
            assert(!((pGloAsgn->IsChlit(Lit1) && (pGloAsgn->IsChlit(Lit2)))));
            if (pGloAsgn->IsChlit(Lit1)) return true;
            if (pGloAsgn->IsChlit(Lit2)) return false;
            return (Lit1 < Lit2);
        }
        return (d1 < d2);
    }
};

struct LitLessByChrono {
    bool operator() (const int& Lit1, const int& Lit2) const {
        ChronoT d1 = pGloAsgn->LitToChrono[Lit1];
        ChronoT d2 = pGloAsgn->LitToChrono[Lit2];
        //if (d1 == d2) {assert(Lit1 == Lit2);}
        return (d1 < d2);
    }
};



map<vector<int>, GsT*> DupLrnTest;

map<vector<int>, LitT> ArgsToGlit;
set<vector<int> > CegLrnSet;

vector<int> HashedApInt(garri a) {
    vector<int> ret = vector<int>(a.arr, a.arr + a.n);
    return ret;
}

vector<int> VectorFromAp(garri a) {
    vector<int> ret = vector<int>();
    foreach (itLit, a) {
        ret.push_back(*itLit);
    }
    return ret;
}



/****************************************************************************/

void AsgnT::init() {
    UndoListByDL =  talloc(garri, pGloProb->LastPlit+1);
    LitToDL =       talloc(uint,        pGloProb->LastPlit+1);
    LitHit  =       garr<double>(pGloProb->LastPlit+1);
    DepByLit =      talloc(AntecedT,    pGloProb->LastPlit+1);
    LitToChrono =   talloc(ChronoT,     pGloProb->LastPlit+1);
    int_to_str =    garr<char*>(pGloProb->LastPlit+1);
    raw_int_to_fmla_lit = garr<Fmla*>(pGloProb->LastPlit+1);
    WaitingPropSet.init(pGloProb->LastPlit + 1);
    for (int path=0; path <= MaxPathNum; path++) {
        PolPref[path] = garr<char>(pGloProb->LastPlit+1);
    }
    //LitToAbsTime =  talloc(long long,   pGloProb->LastPlit+1);
    ChronoToLit =   talloc(LitT,        pGloProb->LastPlit+1);
    LitsInConflict = set<LitT>();
    pConflictGs = NULL;
    pGloAsgn->FreeFmla = NOT_DONE;
    chlits.flat.init();
    LastInitGs = (1 << 30);
    NumAppends = 0;
    for (int i=0; i <= MAX_GST_PRI; i++) {
        PendingExecs[i].init();
    }
    for (int i=0; i <= pGloProb->LastPlit; i++) {
        LitToDL[i] = NO_DECLEV;
        LitToChrono[i] = ((ChronoT) NO_DECLEV);
        // if (IsPosLit(i)) {
        //     asprintf(&int_to_str[i], "%i", i);
        //     raw_int_to_fmla_lit[i] = FmlaVar(int_to_str[i]);
        // }
    }
    this->AppendChlit(ZERO_LIT);
    LitToDL[ZERO_LIT] = ZERO_DECLEV;
    CurChrono = 0;
    
}

void AsgnT::PrintFormula(char const* outname) {
    FILE* outf = fopen(outname, "w");
    fprintf(outf, "CktQBF\n");
    fprintf(outf, "LastInputVar %i\n", absv(pGloProb->LastInpLit));
    fprintf(outf, "LastGateVar %i\n", absv(pGloProb->LastGlit));
    fprintf(outf, "OutputGateLit %i\n\n", absv(pGloProb->OutGlit));
    fprintf(outf, "<q gate=%i>\n", absv(pGloProb->OutGlit));
    for (int i=1; i < pGloProb->QuantBlk.n; i++) {
        QuantBlkT* pQb = &pGloProb->QuantBlk[i];
        assert(pQb->bat == pGloProb->OutGlit);
        fprintf(outf, "    %c", tolower(pQb->qtype));
        foreach (itLit, pQb->lits) {
            fprintf(outf, " %i", decv(*itLit));
        }
        fprintf(outf, "\n\n");
    }
    fprintf(outf, "</q>\n\n");

    for (int g = absv(pGloProb->NextNewVar)-2; g >= pGloProb->FirstGateVar; g -= 2) {
        const char* op = (IsAndGate(g) ? "and" : "or");
        auto(args, dup_as_garr(AbsGlitToGs(g)->ReqLits));
        sort(args.begin(), args.end());
        fprintf(outf, "%i = %s(", g, op);
        bool NoComma = true;
        for (int i=0; i < args.n; i++) {
            int CurArg = args[i];
            if (IsOrGate(g)) {CurArg = negv(CurArg);}
            if (HasLitOrNeg(CurArg)) continue;
            fprintf(outf, "%s%i", (NoComma ? "" : ", "), decv(CurArg));
            NoComma = false;
        }
        args.dealloc();
        fprintf(outf, ")  \n");
    }
    fclose(outf);
}

void AsgnT::PrintFormulaQdimacs(char const* outname) {
    FILE* outf = fopen(outname, "w");
    assert(IsAndGate(pGloProb->OutGlit));
    fprintf(outf, "p cnf 0 0                    \n");
    for (int i=1; i < pGloProb->QuantBlk.n; i++) {
        QuantBlkT* pQb = &pGloProb->QuantBlk[i];
        assert(pQb->bat == pGloProb->OutGlit);
        fprintf(outf, "%c", tolower(pQb->qtype));
        foreach (itLit, pQb->lits) {
            fprintf(outf, " %i", decv(*itLit));
        }
        if (i == pGloProb->QuantBlk.n - 1) {
            if (tolower(pQb->qtype) == 'a') {
                fprintf(outf, " 0\n");
                fprintf(outf, "e ");
            }
            for (int g = pGloProb->FirstGateVar; g < pGloProb->NextNewVar; g += 2) {
                fprintf(outf, " %i", decv(g));
            }
        }
        fprintf(outf, " 0\n");
    }

    int NumClauses = 1;
    fprintf(outf, "%i 0\n", decv(pGloProb->OutGlit));

    for (int g = absv(pGloProb->NextNewVar)-2; g >= pGloProb->FirstGateVar; g = absv(g) - 2) {
        if (IsOrGate(g)) {
            g = negv(g);
        }
        auto(args, dup_as_garr(GlitToGs[g]->ReqLits));
        sort(args.begin(), args.end());
        // (v <==> (x1 & x2 & x3)) expands to
        // (v | ~x1 | ~x2 | ~x3)  &  (~v | x1)  &  (~v | x2)  &  (~v | x3)
        fprintf(outf, "%i ", decv(g));
        for (int i=0; i < args.n; i++) {
            int CurArg = args[i];
            if (HasLitOrNeg(CurArg)) continue;
            fprintf(outf, "%i ", decv(negv(CurArg)));
        }
        NumClauses++;
        fprintf(outf, " 0\n");
        for (int i=0; i < args.n; i++) {
            int CurArg = args[i];
            if (HasLitOrNeg(CurArg)) continue;
            fprintf(outf, "%i %i 0\n", decv(negv(g)), decv(CurArg));
            NumClauses++;
        }
        args.dealloc();
    }
    fflush(outf);
    rewind(outf);
    fprintf(outf, "p cnf %i %i", pGloProb->NextNewVar - 2, NumClauses);
    fclose(outf);
}


// SubstCircuitDefn* AsgnT::NewCiruitDefn() {
//     SubstCircuitDefn* pDef = new SubstCircuitDefn();
//     SubstCircuitDefn& ckt = *pDef;
//     for (int glit = 0; glit < GateDefn.n; glit++) {
//         garri args = GateDefn[glit];
//         if (args.arr == NULL) continue;
//         ckt[glit] = vector<LitT>(args.begin(), args.end());
//         ckt[negv(glit)] = vector<LitT>(1, OR_GATE);
//         foreach (itLit, args) {
//             int lit = *itLit;
//             if (IsInputLit(lit)) {
//                 ckt[lit] = vector<LitT>(1, lit);
//                 ckt[negv(lit)] = vector<LitT>(1, negv(lit));
//             }
//         }
//     }
//     return pDef;
// }

// LitT AsgnT::CktRestrict(SubstCircuitDefn& ckt, int glit) {
//     int ret;
//     if (ckt.count(glit)) {
//         vector<LitT>& sublits = ckt[glit];
//         if (sublits.size() == 1) {
//             return *sublits.begin();
//         } else {
//             return glit;
//         }
//     }
//     if (IsOrGate(glit)) {
//         return negv(CktRestrict(ckt, negv(glit)));
//     }
//     if (IsInputLit(glit)) {
//         ret = glit;
//         ckt[glit] = vector<LitT>(1, glit);
//         return ret;
//     }
//     assert(0 < glit && glit < GateDefn.n);
//     assert(GateDefn[glit].arr != NULL);
//     vector<LitT> sublits = vector<LitT>();
//     foreach (itArg, GateDefn[glit]) {
//         sublits.push_back(CktRestrict(ckt, *itArg));
//     }
//     ckt[glit] = vector<LitT>(sublits.begin(), sublits.end());
//     if (sublits.size() == 1) {
//         return *sublits.begin();
//     } else {
//         return glit;
//     }
// }   

bool AsgnT::HasMoreVars() {
    int align = 4;
    return (pGloProb->NextNewVar + align < pGloProb->LastPlit);
}

int IsInCegar = false;
garr<GsT*> TempNewClauses = garr<GsT*>(0);

int AsgnT::CegarExtend(garri AsgnLits, map<LitT,LitT>* hit, int ElimQb) {
    int NewOut = 0;
    int winr = QtypeNumFromLetr(pGloProb->QuantBlk[ElimQb].qtype);
    assert(winr==0 || winr==1);
    garr<garri> AsgnInQb = garr<garri>(pGloProb->QuantBlk.n);
    timsort(AsgnLits);
    foreach (itLit, AsgnLits) {
        AsgnInQb[QbFromLit[*itLit]].append(*itLit);
    }
    garri AsgnReps = garri(0);
    foreach (itBlk, AsgnInQb) {
        if (true || itBlk->n <= 3) {
            foreach (itLit, *itBlk) {
                AsgnReps.append(*itLit);
            }
        } else {
            AsgnReps.append(NewConj(0, *itBlk));
        }
    }
    assert(GetCurDL() == 0);
    assert(MinPropQb == 0);
    foreach (itLit, AsgnReps) {
        if (HasLit(negv(*itLit))) {
            static int AlreadyWarned = false;
            if (AlreadyWarned == false) {
                AlreadyWarned = true;
                printf("Oh no!  Cegar failed on line %i.\n", __LINE__);
            }
            goto CegarFail;
        }
        if (HasLit(*itLit)) continue;
        if (HasConflict()) break;
        assert(MinPropQb <= QbFromLit[*itLit]);
        MinPropQb = QbFromLit[*itLit];
        AppendChlit(*itLit);
        SchedForProp(*itLit);
        Propagate();
    }

    IsInCegar = true;

    {
    {
    int flag = 1;
    MinPropQb = ElimQb;
    while (flag) {
        flag = 0;
        garr<signed char>& MonoForce = *GetMonoLits();
        for (int lit=0; lit <= pGloProb->RawLastInpLit; lit++) {
            if (HasConflict()) {
                flag = false;
                break;
            }
            if (MonoForce[lit] && MinPropQb <= QbFromLit[lit]) {
                //if (GloDbgLev >= 2000) printf("Mono forced %i\n", lit);
                if (HasLitOrNeg(lit)) continue;
                AppendChlit(lit);
                SchedForProp(lit);
                Propagate();
                flag = true;
            }
        }
        MonoForce.dealloc();
    }
    MinPropQb = 0;
    }
    foreach (itChlit, chlits.flat) {
        garri& CurUndo = UndoListByDL[LitToDL[*itChlit]];
        assert(CurUndo[0] == *itChlit);
        foreach (itLit, CurUndo) {
            int lit = *itLit;
            if (lit == 0) continue;
            if (IsInputLit(lit)) {
                (*hit)[lit] = 1;
                (*hit)[negv(lit)] = 0;
            }
        }
    }
    try {
        if (HasConflict()) {
            goto CegarFail;
        }
        NewOut = Restrict(pGloProb->OutGlit, hit, ElimQb);
    } catch (char const* s) {
        fprintf(stderr, "%s\n", s);
        goto CegarFail;
    }
    restart();
    MinPropQb = 0;
    garri NewArgs = garri(0);
    int NumNewLrn = 0;
    foreach (itCl, TempNewClauses) {
        SimpFixWatch(*itCl);
    }
    max_learnts += NumNewLrn;
    TempNewClauses.n = 0;
    IsInCegar = false;
    NewArgs.dealloc();
    }

    CegarFail: ;
    restart();
    MinPropQb = 0;

    foreach (itQb, AsgnInQb) {
        (*itQb).dealloc();
    }
    AsgnInQb.dealloc();
    AsgnReps.dealloc();
    return NewOut;
}

garr<signed char>* AsgnT::GetMonoLits() {
    static garr<signed char> LitPos;
    static garr<signed char> MonoForce;
    if (HasConflict()) {
        return &MonoForce;
    }
    assert(LitPos.n == 0);
    assert(MonoForce.n == 0);
    MonoForce.resize_bzero(pGloProb->LastOrigGlit + 1);
    LitPos.resize_bzero(pGloProb->LastPlit + 1);
    LitPos[pGloProb->OutGlit] = 1;
    for (int glit = pGloProb->LastOrigGlit; glit > 0; glit--) {
        if (!IsAndGate(glit)) continue;
        if (HasLit(glit) && HasLit(glit ^ 2)) continue;
        if (HasLit(negv(glit)) && HasLit(negv(glit ^ 2))) continue;
        int LiveGlitPos = (LitPos[glit] == 1);
        int LiveGlitNeg = (LitPos[negv(glit)] == 1);
        foreach (itArg, GateDefn[glit]) {
            int arg = *itArg;
            assert(LitPos[arg] != -1);       
            assert(LitPos[negv(arg)] != -1); 
            if (absv(arg) == absv(glit)) abort();
            if (IsInputLit(arg)) assert(!HasLit(negv(arg)));
            assert(arg < glit);
            if (LiveGlitPos) {LitPos[arg] = 1;}
            if (LiveGlitNeg) {LitPos[negv(arg)] = 1;}
        }
        if (!LiveGlitPos) {LitPos[glit] = -1;}
        if (!LiveGlitNeg) {LitPos[negv(glit)] = -1;}
    }
    MonoForce.resize_bzero(pGloProb->LastOrigGlit + 1);
    for (int lit = 0; lit <= pGloProb->RawLastInpLit; lit++) {
        if (HasLitOrNeg(lit)) continue;
        if (LitPos[lit] && !LitPos[negv(lit)]) {
            int flit = (QtypeNumFromLit(lit) ? lit : negv(lit));
            MonoForce[flit] = 1;
        }
    }

    LitPos.n = 0;
    return &MonoForce;
}


int AsgnT::Restrict(int glit, map<LitT,LitT>* hit, int ElimQb) {
    int ret;
    {
    if (hit->count(glit)) {
        if (hit->count(negv(glit))) {
            assert((*hit)[glit] != (*hit)[negv(glit)]);
        }
        return (*hit)[glit];
    }
    if (IsOrGate(glit)) {
        ret = negv(Restrict(negv(glit), hit, ElimQb));
        return ret;
    }
    if (IsInputLit(glit)) {
        if (QbFromLit[glit] < ElimQb) {
            ret = glit;
        } else if (QbFromLit[glit] == ElimQb) {
            ret = 0;
        } else {
            ret = GenNewVar(glit);
            int NewQb = QbFromLit[glit] - 2;
            add_var_to_qblock(absv(ret), NewQb);
            GhostToGs[ret] = INPUT_LIT;
            GhostToGs[negv(ret)] = INPUT_LIT;
            assert(QtypeFromLit(ret) == QtypeFromLit(glit));
            if (GloDbgLev >= 2002) {
                printf("  New CEGAR input var ");
                DumpLitRaw(ret);
                printf(" from ");
                DumpLitRaw(glit);
                printf("\n");
            }
        }
        goto finish_restrict;
    }
    foreach (itArg, GateDefn[glit]) {
        auto(itCached, hit->find(*itArg));
        if (itCached != hit->end()) {
            int NewArg = itCached->second;
            if (NewArg == 0) {
                ret = 0;
                goto finish_restrict;
            }
        }
    }
    garri NewArgs = garri(0,0); 
    bool IsDiff = false;
    foreach (itArg, GateDefn[glit]) {
        int NewArg = Restrict(*itArg, hit, ElimQb);
        if (NewArg == 0) {
            NewArgs.dealloc();
            ret = 0;
            goto finish_restrict;
        }
        if (NewArg == 1) {
            IsDiff = true;
            continue;
        }
        if (NewArg != *itArg) {IsDiff = true;}
        NewArgs.append(NewArg);
    }
    if (NewArgs.n == 0) {
        NewArgs.dealloc();
        ret = 1;
        goto finish_restrict;
    }
    timsort(NewArgs);
    NewArgs.remove_adj_dups();
    for (int i=1; i < NewArgs.n; i++) {
        if (NewArgs.arr[i-1] == negv(NewArgs.arr[i])) {
            NewArgs.dealloc();
            ret = 0;
            goto finish_restrict;
        }
    }
    if (NewArgs.n == 1) {
        ret = NewArgs[0];
        NewArgs.dealloc();
        goto finish_restrict;
    }
    if (!IsDiff) {
        NewArgs.dealloc();
        ret = glit;
        goto finish_restrict;
    }
    ret = NewConj(glit, NewArgs);
    NewArgs.dealloc();
    }

    finish_restrict:
    //printf("%i => %i\n", glit, ret);
    assert(hit->count(glit) == 0);
    assert(hit->count(negv(glit)) == 0);
    (*hit)[glit] = ret;
    (*hit)[negv(glit)] = negv(ret);
    return ret;
}

garr<GsT*> KillTries;


GsT* AsgnT::Augment(int glit, garri AsgnLits, map<LitT,LitT>* hit, int ElimQb) {
    int winr = QtypeNumFromLetr(pGloProb->QuantBlk[ElimQb].qtype);
    assert(glit == pGloProb->OutGlit);
    foreach (itLit, AsgnLits) {
        assert(QbFromLit[*itLit] <= ElimQb);
        if (QbFromLit[*itLit] != ElimQb) continue;
        (*hit)[*itLit] = 1;
        (*hit)[negv(*itLit)] = 0;
    }
    int NewOut = CegarExtend(AsgnLits, hit, ElimQb);
    garri args = garri(0, 16);
    int WinrOut = -1, LosrOut = -1;
    int FrobOut = (winr==1 ? NewOut : negv(NewOut));
    if (FrobOut == 0) {
        if (GloDbgLev >= 2002) {
            printf("WARNING: Opposing player won in CEGAR learning!\n");
            foreach (itPair, *hit) {
                if (IsInputLit(itPair->first)) {
                    if (itPair->first == itPair->second) continue;
                    int OrigLit = (itPair->first);
                    assert((*hit)[OrigLit] != (*hit)[negv(OrigLit)]);
                    DumpLitRaw(itPair->first);
                    printf(": ");
                    printf("%i", itPair->second);
                    printf(",  ");
                }
            }
            printf("\n");
        }
        args.dealloc();
        return 0;
    } else if (FrobOut == 1) {
        args.n = 0;
    } else {
        if (IsReallyGateLit(FrobOut)) {
            LosrOut = ghosted(FrobOut, winr ^ 1);
            WinrOut = ghosted(FrobOut ^ 1, winr);
        } else {
            LosrOut = FrobOut;
            WinrOut = FrobOut;
        }
        args.append(LosrOut);
    }
    foreach (itPair, *hit) {
        int lit = itPair->first;
        int NewLit = itPair->second;
        if (NewLit != 1) continue;
        if (ElimQb <= QbFromLit[lit]) continue;
        args.append(lit);
    }
    vector<int> entry = vector<int>(args.arr, args.arr + args.n);
    if (CegLrnSet.count(entry)) {
        args.dealloc();
        return 0;
    } 
    CegLrnSet.insert(entry);
    GsT* ret = NULL;

    if (FrobOut == 1) {
        static int AlreadyWarned = false;
        if (AlreadyWarned == false) {
            AlreadyWarned = true;
            printf("WARNING: Case where FrobOut==1 is not implemented, aborting CEGAR!\n");
        }
    }

    garri WinrGuard = garri(0);
    foreach (itLit, AsgnLits) {
        if (QbFromLit[*itLit] != ElimQb) {
            WinrGuard.append(*itLit);
        }
    }

    if (FrobOut != 1) {
        WinrGuard.append(LosrOut);
        ret = NewLrnGs_Mixed(WinrGuard, winr);
        assert(args[0] == LosrOut);
        args[0] = WinrOut;
    }

    args.dealloc();
    WinrGuard.dealloc();
    return ret;
}

int AsgnT::NewConj(int OrigGate, garri args) {
    timsort(args);
    args.remove_adj_dups();
    if (args.n == 1) {return args[0];}
    auto(entry, HashedApInt(args));
    if (ArgsToGlit.count(entry)) {
        int ret = ArgsToGlit[entry];
        return ret;
    }
    int ret = GenNewVar(OrigGate, 4);
    add_gate_to_qb(ret, args);
    NewDefnGsAlt(ret, args);
    if (GloDbgLev >= 2002) {
        printf("  New CEGAR gate ");
        DumpLitRaw(ret);
        printf(" from ");
        DumpLitRaw(OrigGate);
        printf(", def_gs: %4i", GhostToGs[ret]->id);
        printf(", args: ");
        DumpLitsEnc(args, "");
        printf("\n");
    }
    assert(ArgsToGlit.count(entry));
    if (1) {
        auto(test, HashedApInt(args));
        assert(ArgsToGlit.count(test));
    }
    return ret;
}


int AsgnT::GenNewVar(int orig, int align=2) {
    if (align == 4 and pGloProb->NextNewVar % 4 != 0) {
        pGloProb->NextNewVar += 2;
    }
    int ret = pGloProb->NextNewVar;
    if (ret + align > pGloProb->LastPlit) {
        throw "Out of space for new variables!";
    }
    assert(QbFromLit[ret] == 0);
    assert(ret % align == 0);
    pGloProb->NextNewVar += align;
    ret = ret ^ signv(orig);
    return ret;
}





/*****************************************************************************
* Functions dealing mainly with game-state objects.    }}}{{{1
*****************************************************************************/

int GsT::GetWinner() {
    if (FreeFmla == FmlaTrue) return 'E';
    if (FreeFmla == FmlaFalse) return 'A';
    abort();
}

int GsT::HasBlockedImps() {
    for (int i=0; i < ImpLits.n; i++) {
        if (pGloAsgn->HasLit(negv(ImpLits[i]))) {
            return ImpLits[i];
        }
    }
    return false;
}

ap<LitT>* GsT::GetWinrLits(int winr) {
    GsT* pCurLs = this;
    return &pCurLs->ImpLits;
}

int AsgnT::GetUpstreamMissingLit(int ForcedLit, GsT* pCurLs) {
    return GetUpstreamMissingLitAp(ForcedLit, pCurLs->ImpLits);
}

int AsgnT::GetUpstreamMissingLitAp(int flit, ap<int> LitList) {
    int WatchQb = QbFromLit[flit];
    for (int i=0; i < LitList.n; i++) {
        int CurLit = LitList[i];
        if (QbFromLit[CurLit] < WatchQb && !pGloAsgn->HasLit(CurLit)) {
            return CurLit;
        }
    }
    return 0;
}

int AsgnT::GetFirstFreeLit(ap<int>& LitList) {
    for (int i=0; i < LitList.n; i++) {
        int CurLit = LitList[i];
        if (!pGloAsgn->HasLit(CurLit)) {
            return CurLit;
        }
    }
    return 0;
}

int GsT::HasAllWatchedTrigs() {
    for (int i=0; i < 2; i++) {
        if (!pGloAsgn->HasLit(WatchRawAE[i])) return false;
    }
    return true;
}

int GsT::TrigRipe() {
    if (!pGloAsgn->HasLit(WatchLosrAE(1))) {return 0;}
    if (pGloAsgn->HasLit(negv(WatchLosrAE(0)))) {return 0;}
    if (pGloAsgn->HasLit(WatchLosrAE(0))) {return 2;}
    return 1;
}

int GsT::Is_ExactlyOne_LosrLit_Free() {
    return (pGloAsgn->HasLit(WatchLosrAE(0)) ^ pGloAsgn->HasLit(WatchLosrAE(1)));
}

void GsT::InitTrigLits(ap<int> args) {
    int MaxReqQb = 0, MaxFutQb = 0;
    foreach (itLit, ReqLits) {
        MaxReqQb = max(MaxReqQb, (int)QbFromLit[*itLit]);
    }
    foreach (itLit, ImpLits) {
        MaxFutQb = max(MaxFutQb, (int)QbFromLit[*itLit]);
    }
    //assert(MaxReqQb==0 || MaxFutQb==0 || MaxFutQb < MaxReqQb);
}

int GsT::PrintIndex() {
    // This is for debugging.
    int ret = pGloProb->clauses.indexof(this);
    printf(  "pGloProb->clauses.arr[%i]\n", ret);
    return ret;
}

void GsT::RegisterLitsHave() {
    for (int i=0; i < ReqLits.n; i++) {
        int lit = ReqLits[i];
        pGloProb->LitHave[lit].append(this);
    }
}

void GsT::dealloc() {
    assert(!InUse);
    assert(pGloProb->clauses[this->id] == this);
    TempLearntClauses.erase(this);
    auto(pRawInts, this->AsLitVec());
    DupLrnTest.erase(*pRawInts);
    delete pRawInts;
    pGloAsgn->EraseWatchedLits(this);
    this->ReqLits.dealloc();
    this->ImpLits.dealloc();
    pGloProb->clauses[this->id] = NULL;
}

bool GsT::is_subsumed_by(GsT* pOther) {
    if (this->ReqLits.n < pOther->ReqLits.n) {
        return false;
    }
    auto(cur1,  pOther->ReqLits.begin());
    auto(last1, pOther->ReqLits.end());
    auto(cur2,  this->ReqLits.begin());
    auto(last2, this->ReqLits.end());
    while (cur1 != last1 && cur2 != last2) {
        if (*cur1 < *cur2) {return false;}
        else if (*cur2 < *cur1) {cur2++;}
        else { cur1++; cur2++; }
    }
    return true;
}

void GsT::bump_activity() {
    this->activity += cla_inc;
    if (this->activity > 1e20) {
        pGloAsgn->cut_clause_act();
    }
}

FullStratT* GsT::get_strat() {
    return pGloProb->clause_id_to_strat.getval(this->id, NULL);
}

Fmla* GsT::get_strat_as_list_fmla() {
    return strat_to_list_fmla(this->get_strat());
}

void GsT::set_strat(FullStratT* strat) {
    assert(this->id != 0);
    pGloProb->clause_id_to_strat.setval(this->id, strat);
}

void GsT::dump_strat() {
    dump_fmla(strat_to_list_fmla(get_strat()));
}

void AsgnT::bump_var_act(LitT var) {
    assert(absv(var) == var);
    VarActivity[var] += var_inc;
    if (VarActivity[var] > 1e20) {
        cut_var_act();
    }
    if (VarActOrder.inHeap(var)) {
        VarActOrder.decrease_key(var);
    }
}

void AsgnT::InsertVarActOrder(LitT var) {
    if (!VarActOrder.inHeap(var)) {
        VarActOrder.insert(var);
    }
}


/*****************************************************************************
* Stuff  }}}{{{1
*****************************************************************************/

GsT* GsById(int id) {
    /* This function is for use in the debugger. */
    return pGloProb->clauses[id];
}

inline int AsgnT::GetWinner() {
    if (FreeFmla == FmlaTrue) return 'E';
    if (FreeFmla == FmlaFalse) return 'A';
    else return 'F';
    //abort();
}


void AsgnT::cut_clause_act() {
    foreach (it, pGloProb->clauses) {
        GsT* pCurLs = *it;
        if (!pCurLs) continue;
        pCurLs->activity *= 1e-20;
    }
    cla_inc *= 1e-20;
    if (cla_inc < 1e-30) {
        cla_inc = 1e-30;
    }
}

void AsgnT::cut_var_act() {
    for (int var=0; var <= pGloProb->LastPlit; var += 2) {
        VarActivity[var] *= 1e-20;
    }
    var_inc *= 1e-20;
    if (var_inc < 1e-30) {
        var_inc = 1e-30;
    }
}

void AsgnT::cut_lit_hit() {
    for (int lit=0; lit <= pGloProb->LastPlit; lit += 1) {
        LitHit[lit] *= 1e-20;
    }
    LitHitInc *= 1e-20;
    if (LitHitInc < 1e-30) {
        LitHitInc = 1e-30;
    }
}

void AsgnT::cla_decay_activity() {
    double clause_decay = 1.003;
    cla_inc *= clause_decay;
}

void AsgnT::var_decay_activity() {
    double var_decay = 1.05;
    var_inc *= var_decay;
}

void DumpIntSet(set<int,struct LitLessByChrono>* pSet, const char* pEnd) {
    auto(iter, pSet->begin());
    printf("[ ");
    for (; iter != pSet->end(); ++iter) {
        printf("%i ", *iter);
    }
    printf("]%s", pEnd);
}

double AsgnT::get_median_clause_adj_activity() {
    garr<double> acts = garr<double>();
    foreach (it, TempLearntClauses) {
        GsT* pCurLs = *it;
        if (pCurLs->InUse) continue;
        acts.append(pCurLs->get_adj_act());
    }
    double median = acts.find_median_destructively();
    acts.dealloc();
    return median;
}

void AsgnT::delete_excess_learned_clauses() {
    int NumLearned = TempLearntClauses.size() - NumLearntLocked;
    if (NumLearned < max_learnts) return;
    max_learnts += (max_learnts / 128) + 1;
    double median = get_median_clause_adj_activity();
    garr<GsT*> killset = garr<GsT*>();
    foreach (it, TempLearntClauses) {
        GsT* pCurLs = *it;
        if (pCurLs->InUse) continue;
        if (pCurLs->get_adj_act() < median) {
            killset.append(pCurLs);
        }
    }
    int NumDel = 0;
    foreach (it, killset) {
        GsT* pCurLs = *it;
        if (pCurLs->InUse) continue;
        if (GloDbgLev >= 2000) {
            int time_unused = NumBigBt - pCurLs->LastUse;
            double norm_act = pCurLs->activity / cla_inc;
            printf("Deleting sequent %6i (size %4i, NumWatFix %4i, norm_act %#7.2e, adj_act %7.2e, last use %5i ago).\n", 
                pCurLs->id, pCurLs->ReqLits.n, pCurLs->NumFixes, norm_act, pCurLs->get_adj_act(), time_unused);
        }
        NumDel++;
        TempLearntClauses.erase(*it);
        pCurLs->dealloc();
    }
    killset.dealloc();
    if (GloDbgLev >= 1900) {
        printf("Deleted %i of %i available sequents, norm_cutoff = %.2e.\n", 
            NumDel, NumLearned, median / cla_inc);
    }
}


void GsT::dump() {
    DumpRaw();
}

void GsT::DumpBrief() {
    printf("  ReqLits "); DumpLitsEnc(ReqLits, ",  ");
    printf("  Imps "); DumpLitsEnc(ImpLits, "\n");
}

void GsT::DumpRaw() {
    if (this == NULL) {printf("(null)\n"); return;}
    if (this == (GsT*)(1)) {printf("(GsT *)(0x1)\n"); return;}
    if (pGloProb->clauses[id] != this) {printf("############ DEAD ##############\n");}
    printf("pGloProb->clauses.arr[%i] (%s).  ", id, (IsLearned ? "learned" : "original"));
    char WinrLetr = '-';
    if (FreeFmla == FmlaTrue) {WinrLetr = 'E';}
    if (FreeFmla == FmlaFalse) {WinrLetr = 'A';}
    printf("winner='%c'.\n", WinrLetr);
    printf("InUse = %i, LastUse = %u, activity = %#.4g\n", InUse, LastUse, activity);
    // printf("GateLit=%i.\n", GateLit);
    printf("ReqLits: "); DumpLitsEnc(ReqLits);
    printf("ImpLits: "); DumpLitsEnc(ImpLits);
    printf("Watch: [%i, %i], %i.\n", 
        decv(WatchLosrAE(0)), decv(WatchLosrAE(1)),
        decv(WatchedRes));
    if (FreeFmla->num_args > 0) {
        printf("FreeFmla: ");
        this->FreeFmla->simp_ite()->write(stdout);
        printf("\n");
    }
}


        
int AsgnT::ChlitByDL(int dl) {
    return chlits.flat[dl];
}

int AsgnT::QbFromDL(int dl) {
    return QbFromLit[ChlitByDL(dl)];
}

int AsgnT::QtypeFromDL(int dl) {
    return GloQtype(QbFromDL(dl));
}

int AsgnT::GlitToMinChrono(int lit, int ply) {
    int ret = LitToChrono[lit];
    return ret;
}



/*****************************************************************************
* Functions for pending queue of game-states.  }}}{{{1
*****************************************************************************/
void AsgnT::AddPendingLs(GsT* pCurLs, int score) {
    PendingExecs[score].append(AntecedT(pCurLs));
}

AntecedT AsgnT::PopFirstPendingLs(int flag) {
    if (!PendingExecs[CONFLICT_GST_PRI].is_empty()) {
        return PendingExecs[CONFLICT_GST_PRI].pop();
    }
    if (flag) return AntecedT();
    if (!PendingExecs[FORC_GST_PRI].is_empty()) {
        return PendingExecs[FORC_GST_PRI].pop();
    }
    return AntecedT();
}

bool AsgnT::HasPendingForced() {
    if (!PendingExecs[CONFLICT_GST_PRI].is_empty()) return true;
    if (!PendingExecs[FORC_GST_PRI].is_empty()) return true;
    return false;
}


/*****************************************************************************
* Functions for testing if a game-state matches yet.  }}}{{{1
*****************************************************************************/
int AsgnT::HasConflict() {
    int ret = (pConflictGs != NULL);
    assert(ret == (FreeFmla != NOT_DONE));
    return ret;
}


/*****************************************************************************
* Functions for watched literals.  }}}{{{1
*****************************************************************************/

extern inline
int FixOneWatchedLit(GsT* pCurLs, int OldLit) {
    int iOld;
    if      (pCurLs->WatchRawAE[0] == OldLit) {iOld = 0;}
    else if (pCurLs->WatchRawAE[1] == OldLit) {iOld = 1;}
    else {
        assert(OldLit == pCurLs->WatchedRes);
        pGloAsgn->FixWatchedRes(pCurLs);
        return 0;
    }
    int OtherLit = pCurLs->WatchRawAE[1 - iOld];

    if (LitToDL[negv(OtherLit)] != NO_DECLEV) {
        return 0xDEAD;
    }

    NumWatchFixes++;

    if (pCurLs->WatStopBt < NumStepBt) {
        pCurLs->WatStopBt = NumStepBt;
        pCurLs->WatStartPos = 0;
    }
    int NumLits  =  pCurLs->ReqLits.n;
    int* PossLits = pCurLs->ReqLits.arr; // NOTE!!! ReqLits changed in clean_up_sequents
    int ixLit;
    for (ixLit = 0; ixLit < pCurLs->WatStartPos; ixLit++) {
        int CurLit = PossLits[ixLit];
        assert(!(LitToDL[CurLit] == NO_DECLEV && CurLit != OtherLit));
    }
    for (ixLit = pCurLs->WatStartPos; ixLit < NumLits; ixLit++) {
        int CurLit = PossLits[ixLit];
        if (LitToDL[CurLit] == NO_DECLEV && CurLit != OtherLit) {
            pGloAsgn->LitWatchErase(OldLit, pCurLs, pCurLs->itWatchRawAE[iOld]);
            pCurLs->itWatchRawAE[iOld] = pGloAsgn->LitWatchInsert(CurLit, pCurLs);
            pCurLs->WatchRawAE[iOld] = CurLit;
            //pCurLs->WatStartPos = ixLit;
            return 0;
        }
    }
    //pCurLs->WatStartPos = ixLit;
    if (iOld==0 && LitToDL[OtherLit] == NO_DECLEV) {
        swap(pCurLs->WatchRawAE[0], pCurLs->WatchRawAE[1]);
        swap(pCurLs->itWatchRawAE[0], pCurLs->itWatchRawAE[1]);
    }
    return 0;
}


int FixOneWatchedLitBig(GsT* pCurLs, int OldLit) __attribute__ ((noinline));
int FixOneWatchedLitBig(GsT* pCurLs, int OldLit) {
    return FixOneWatchedLit(pCurLs, OldLit);
}

int FixOneWatchedLitSmall(GsT* pCurLs, int OldLit) __attribute__ ((noinline));
int FixOneWatchedLitSmall(GsT* pCurLs, int OldLit) {
    return FixOneWatchedLit(pCurLs, OldLit);
}

inline void AsgnT::FastFixWatch(GsT* pCurLs, int OldLit) {
    int IsRes = (OldLit == pCurLs->WatchedRes);
    int ok;
    if (pCurLs->ReqLits.n > 20) {
        ok = FixOneWatchedLitBig(pCurLs, OldLit);
    } else {
        ok = FixOneWatchedLitSmall(pCurLs, OldLit);
    }
    assert(pCurLs->WatStartPos == pCurLs->ReqLits.n || pCurLs->ReqLits[pCurLs->WatStartPos] <= max(pCurLs->WatchRawAE[0], pCurLs->WatchRawAE[1]));
    if (ok == 0xDEAD) {return;}
    if (!IsRes) {
        FixWatchedRes(pCurLs);
    }
}

void AsgnT::SimpFixWatch(GsT* pCurLs) {
    int OldWat[2] = {pCurLs->WatchRawAE[0], pCurLs->WatchRawAE[1]};
    if (pCurLs->ReqLits.n >= 1 && HasLit(OldWat[0])) {FixOneWatchedLitSmall(pCurLs, OldWat[0]);}
    if (pCurLs->ReqLits.n >= 2 && HasLit(OldWat[1])) {FixOneWatchedLitSmall(pCurLs, OldWat[1]);}
    FixWatchedRes(pCurLs);
}

void AsgnT::FixWatchedRes(GsT* pCurLs) {
    int ResLitW = 0;
    //if (pCurLs->ImpLits.n > 0 && pCurLs->Is_ExactlyOne_LosrLit_Free()) 
    if (pCurLs->ImpLits.n > 0 && pCurLs->Is_ExactlyOne_LosrLit_Free()) {
        int LastTrig = pCurLs->WatchLosrAE(0);
        ResLitW = GetFirstFreeLit(pCurLs->ImpLits);
        int OldRes = pCurLs->WatchedRes;
        if (OldRes != ResLitW && ResLitW != 0) {
            if (QbFromLit[ResLitW] < QbFromLit[LastTrig]) {
                #ifndef NO_SAN_CHK
                assert(ResLitW == PickNewResWatch(pCurLs));
                #endif
                if (OldRes != 0) {LitWatchErase(OldRes, pCurLs, pCurLs->itWatchRes);}
                pCurLs->itWatchRes = LitWatchInsert(ResLitW, pCurLs);
                pCurLs->WatchedRes = ResLitW;
            }
        }
    }
}

void AsgnT::EraseWatchedLits(GsT* pCurLs) {
    for (int i=0; i < 2; i++) {
        int cur = pCurLs->WatchRawAE[i];
        if (cur==0) continue;
        LitWatchErase(cur, pCurLs, pCurLs->itWatchRawAE[i]);
    }
    for (int i=0; i < 2; i++) {
        pCurLs->WatchRawAE[i] = 0;
    }
    int OldRes = pCurLs->WatchedRes;
    if (OldRes != 0) {
        LitWatchErase(OldRes, pCurLs, pCurLs->itWatchRes);
        pCurLs->WatchedRes = 0;
    }
}


int AsgnT::PickNewResWatch(GsT* pCurLs) {
    int ResLitW = 0;
    if (!pCurLs->Is_ExactlyOne_LosrLit_Free()) return 0;
    int LastTrig = pCurLs->WatchLosrAE(0);
    ResLitW = GetFirstFreeLit(pCurLs->ImpLits);
    if (ResLitW==0) return 0;
    if (QbFromLit[ResLitW] < QbFromLit[LastTrig]) {return ResLitW;}
    else return 0;
}

void AsgnT::ChangeWatchedFut(GsT* pCurLs, int NewFut) {
    int OldFut = pCurLs->WatchedRes;
    if (OldFut == NewFut) return;
    if (OldFut != 0) {pGloAsgn->LitWatchErase(OldFut, pCurLs, pCurLs->itWatchRes);}
    pCurLs->itWatchRes = pGloAsgn->LitWatchInsert(NewFut, pCurLs);
    pCurLs->WatchedRes = NewFut;
}


/*****************************************************************************
* Stuff  }}}{{{1
*****************************************************************************/

int AsgnT::Backtrack(bool KeepDepsLocked=false) {
    LastPickGate = 0;
    GloStep++;
    NumStepBt++;
    int UndoChlit = GetLastChlit();
    uint CurDL = LitToDL[UndoChlit];
    if (GloDbgLev >= 2100) printf("Popping %i (DL %i) (step %i)\n", decv(UndoChlit), CurDL, GloStep);
    assert(CurDL != ZERO_DECLEV);

    int UndoDL = LitToDL[UndoChlit];
    garri& CurUndoList = UndoListByDL[UndoDL];
    
    (UndoChlit == chlits.pop()) || die();

    bool qPrintRemoved = (GloDbgLev >= 2110);
    if (qPrintRemoved) printf("Removing [ ");
    for (int i = CurUndoList.n - 1; i >= 0; i--) {
        int flit = CurUndoList[i];
        if (qPrintRemoved) printf("%i ", decv(flit));
        assert(LitToDL[flit] == CurDL);
        LitToDL[flit] = NO_DECLEV;
        LitToChrono[flit] = ((ChronoT) NO_DECLEV);
        PolPref[CurPathNum][absv(flit)] = (1 ^ signv(flit));
        InsertVarActOrder(absv(flit));
        CurChrono--;
        ResetDepByLit(flit, KeepDepsLocked);
    }
    if (pConflictGs) {
        int HasAll = HasLit(pConflictGs->WatchRawAE[0]);
        if (!HasAll) {
            pConflictGs->InUse &= ~CurPathMask;
            pConflictGs = NULL;
            pGloAsgn->FreeFmla = NOT_DONE;
            LitsInConflict.clear();
        }
    }

    for (int i=0; i <= MAX_GST_PRI; i++) {
        PendingExecs[i].clear();
    }
    if (qPrintRemoved) printf("]\n");
    UndoListByDL[UndoDL].n = 0;
    
    return UndoChlit;
}


void AsgnT::AddUndoLit(int chlit, int flit) {
    int UndoDL = LitToDL[chlit];
    //if (GloSanChk > 0) assert(UndoListByDL[UndoDL].indexof(flit) == -1);
    UndoListByDL[UndoDL].append(flit);
    LitToChrono[flit] = CurChrono;
    ChronoToLit[CurChrono] = flit;
}

garri& AsgnT::GetCurUndoList() {
    return UndoListByDL[GetCurDL()];
}

void AsgnT::AppendChlit(int NewChlit, bool IsRedo=false) {
    CurChrono++;
    NumAppends++;
    assert(!HasLit(negv(NewChlit)));
    assert(!HasLit(NewChlit));
    assert(GetDepByLit(NewChlit)== NULL);
    //assert(chlits.flat.n > 0 && QbFromLit[NewChlit] >= QbFromLit[GetLastChlit()]);
    uint declev = chlits.flat.n;
    if (GloDbgLev >= 2005 && !IsRedo) {
        printf("Chose Lit %i (at DL #%i)\n", decv(NewChlit), declev);
    }
    chlits.append(NewChlit);
    LitToDL[NewChlit] = declev;
    LitToChrono[NewChlit] = CurChrono;
    //LitToAbsTime[NewChlit] = NumAppends;
    ChronoToLit[CurChrono] = NewChlit;
    UndoListByDL[declev].n = 0;
    UndoListByDL[declev].append(NewChlit);
    assert(ChlitByDL(declev) == NewChlit);
    if ((LitHit[NewChlit] += LitHitInc) > 1e20) {cut_lit_hit();}
    //LastHitDL[declev] = *pNumClauses;
}

void AsgnT::AppendForcedLit(int lit, GsT* pRef) {
    CurChrono++;
    NumAppends++;
    uint NewDL = GetCurDL();
    int UndoChlit = ChlitByDL(NewDL);
    assert(UndoChlit == GetLastChlit());
    assert(!HasLit(negv(lit)));
    assert(!HasLit(lit));
    //assert(pRef->HasLit(negv(lit)) || pRef->HasLit(lit) || pRef->GateLit==lit);
    SetDepByLit(lit, pRef);
    AddUndoLit(UndoChlit, lit);
    LitToDL[lit] = LitToDL[UndoChlit];
    //LitToAbsTime[lit] = NumAppends;
    if (GloDbgLev >= 2051) {
        int doit = (GloDbgLev >= 2050 || IsInputLit(lit));
        if (doit) {
            //assert(QtypeFromLit(lit) != 'F');
            printf("Appending literal %4i: forced at DL %i (chlit %i) ", 
                    decv(lit), LitToDL[UndoChlit], decv(UndoChlit));
            if (pRef) {printf("by clause[%i].\n", pRef->id);}
            else {printf("by something other than unit prop.\n");}
        }
    }
    if ((LitHit[lit] += LitHitInc) > 1e20) {cut_lit_hit();}
}





/*****************************************************************************
* Stuff  }}}{{{1
*****************************************************************************/
void AsgnT::RandomizePolPref() {
    for (int lit=0; lit <= pGloProb->LastPlit; lit++) {
        PolPref[CurPathNum][lit] = rand() % 2;
    }
}
    
int AsgnT::ChooseRandLit(int preferred, bool NoExpu=false) {
    int UseVsids = 0;
    rand();  // Vestigial; kept to sync RNG to old version of code.
    if (rand() % 64 != 0 && NumBigBt > 32) {
        UseVsids = 1;
    }

    NumDecs++;
    if (UseVsids) {
        int ret = 0;
        while (ret == 0) {
            ret = VarActOrder.removeMin();
            if (HasLitOrNeg(ret)) {ret = 0;}
        }
        assert(absv(ret) == ret);
        return ret ^ (1 ^ PolPref[CurPathNum][ret]);
    }

    static garri choices;
    choices.n = 0;
    QuantPfxT CurPfx = QuantPfx;
    assert(CurPfx.blks.n != 0);
    int PrevQb = QbFromLit[GetLastChlit()];
    int CurQb = 0;

    if (!HasLitOrNeg(preferred)) {
        choices.n = 1;
        choices[0] = preferred;
        goto ChooseUp;
    }

    foreach (itQb, pGloProb->QuantBlk) {
        if (itQb->ix < PrevQb) continue;
        foreach (itLit, itQb->lits) {
            int lit = *itLit;
            if (CurQb != 0) {
                if (QbFromLit[lit] != CurQb) {break;};
            } 
            if (HasLitOrNeg(lit)) {
                continue;
            }
            if (CurQb == 0) {
                CurQb = QbFromLit[lit];
            }
            choices.append(lit);
            choices.append(negv(lit));
        }
    }

    int chosen;
    ChooseUp:
    {
        int x = rand();
        if (VarOrdFix) {x = 0;}
        if (choices.n == 0) {
            return 0;
        }
        chosen = choices[x % choices.n];
    }
    return chosen;
}

void AsgnT::DumpCurUndo() {
    printf("%i forced lits: ", GetCurUndoList().n);
    DumpLitsEnc(GetCurUndoList());
}

void AsgnT::dump(int GatesToo) {
    printf("Lits:\n");
    garri& blk = chlits.flat;
    int nch = 0;
    for (int i=0; i < blk.n; i++) {
        int chlit = blk[i];
        uint dl = LitToDL[chlit];
        nch = printf("    %6i (QB %i%c, DL %2i): ", decv(chlit), QbFromLit[chlit], tolower(QtypeFromLit(chlit)), dl);

        garri& CurUndo = UndoListByDL[LitToDL[chlit]];
        if (CurUndo.n == 0) {
            printf("(none)\n");
        } else {
            const char* sep = "";
            printf("[");
            foreach (it, CurUndo) {
                if (!GatesToo && IsReallyGateLit(*it)) continue;
                nch += printf("%s", sep);
                nch += DumpLitRaw(*it);
                if (nch > LINE_LEN - 7) {
                    if (nch < LINE_LEN) printf("\n");
                    nch = 0;
                }
                sep = " ";
            }
            printf("]\n");
        }
    }
}
void AsgnT::dump() {
    dump(false);
}

void AsgnT::DumpChlits() {
    garri& blk = chlits.flat;
    printf("[ ");
    for (int i=0; i < blk.n; i++) {
        int chlit = blk[i];
        if (chlit != 0) printf("%i ", decv(chlit));
    }
    printf("]\n");
}

void AsgnT::DumpLits(int narf) {
    garri& blk = chlits.flat;
    printf("[ ");
    for (int i=0; i < blk.n; i++) {
        int chlit = blk[i];
        printf("[ ");
        garri& CurUndo = UndoListByDL[LitToDL[chlit]];
        foreach (it, CurUndo) {
            printf("%i ", *it);
        }
        printf("] ");
    }
    printf("]\n");
}

void AsgnT::DumpAsgn() {
    garri& blk = chlits.flat;
    printf("[ ");
    for (int i=0; i < blk.n; i++) {
        int chlit = blk[i];
        garri& CurUndo = UndoListByDL[LitToDL[chlit]];
        foreach (it, CurUndo) {
            printf("%i ", *it);
        }
    }
    printf("]\n");
}


void AsgnT::DumpQuantPfx() {
    // For debugging.
    foreach (it, pGloProb->QuantBlk) {
        printf("%2i(%c): ", it->ix, it->qtype);
        DumpLitsEnc(it->lits);
    }
}


void AsgnT::restart() {
    while (chlits.flat.n > 1) {
        Backtrack();
    }
    clean_up_sequents();
}

void AsgnT::clean_up_sequents() {
    if (AllowFree) {
        return;
    }
    assert(GetCurDL() == 0);
    foreach (itCl, pGloProb->clauses) {
        GsT* pCurLs = *itCl;
        if (!pCurLs) continue;
        if (pCurLs->InUse) continue;
        bool RemovedLit = false;
        foreach (itLit, pCurLs->ReqLits) {
            if (HasLit(negv(*itLit))) {
                pCurLs->dealloc();
                break;
            }
            if (HasLit(*itLit) && *itLit != pCurLs->WatchRawAE[0] && *itLit != pCurLs->WatchRawAE[1]) {
                *itLit = 0;
                // NOTE: Must also remove it from WatOrd.
                RemovedLit = true;
            }
        }
        if (RemovedLit) {
            pCurLs->ReqLits.remove_value(0);
            pCurLs->WatStartPos = 0;
            pCurLs->WatStopBt = 0;
        }
    }
}



/*****************************************************************************
* Sanity Check for WatchedLits.    }}}{{{1
*****************************************************************************/
void AsgnT::WatchSanityChk1(GsT* pCurLs) {
        if (pCurLs->inactive) {
            assert(pCurLs->WatchRawAE[0] == 0);
            return;
        }
        int* OldAE = pCurLs->WatchRawArr;
        if (HasLit(OldAE[0]) || HasLit(OldAE[1])) {
            int NumFree = 0;
            foreach (itLit, pCurLs->ReqLits) {
                if (HasLit(negv(*itLit))) {return;}
                if (HasLit(*itLit)) {NumFree++;}
            }
        }
        if (HasLit(OldAE[0]) || HasLit(OldAE[1])) {
            int OldRes = pCurLs->WatchedRes;
            int NewRes = PickNewResWatch(pCurLs);
            if (NewRes != 0 && OldRes != NewRes) {
                if (!HasLit(NewRes)) assert(!HasLit(OldRes));
            }
        }
}
void AsgnT::WatchSanityChk3() {
    for (int i=1; i < pGloProb->clauses.n; i++) {
        GsT* pCurLs = pGloProb->clauses[i];
        if (pCurLs == NULL) continue;
        if (pCurLs->ReqLits.n == 1) continue;
        WatchSanityChk1(pCurLs);
    }
}

void AsgnT::LatentExecsChk() {
    for (int i=1; i < pGloProb->NumDoneClauses; i++) {
        GsT* pCurLs = pGloProb->clauses[i];
        if (pCurLs == NULL) continue;
        if (pCurLs->inactive) {
            assert(pCurLs->WatchRawAE[0] == 0);
            continue;
        }
        int ret = ExecLitSet(pCurLs, EM_DRYRUN);
        assert(ret <= 0);
    }
}

short AsgnT::ReEval(int v) {
    const short UNKNOWN = 7000;
    const short UNASGN  =  900;
    if (v == 0) {
        for (int i=0; i <= pGloProb->LastPlit; i++) {
            ReEvalArr[i] = UNKNOWN;
        }
        ReEval(pGloProb->OutGlit);
        return 0;
    }
    assert(v != 206);
    if (IsInputLit(v)) {
        int ret;
        if (HasLit(v)) ret = 1;
        else if (HasLit(negv(v))) ret = 0;
        else ret = UNASGN;
        ReEvalArr[v] = ret;
        return ret;
    }
    if (ReEvalArr[v] != UNKNOWN) {
        return ReEvalArr[v];
    }
    if (IsOrGate(v)) {
        int ret = negv(ReEval(negv(v)));
        if (ret > 1) {ret = absv(ret);}
        ReEvalArr[v] = ret;
        return ret;
    }
    GsT* pCurLs = GlitToGs[v];
    int ret = 1;
    foreach (itLit, pCurLs->ReqLits) {
        int val = ReEval(*itLit);
        if      (val == UNASGN) {ret = UNASGN;}
        else if (val == 0) {ret = 0; break;}
        else {assert(val == 1);}
    }
    ReEvalArr[v] = ret;
    return ret;
}


/*****************************************************************************
* Functions for Boolean Constraint Propagation proper.    }}}{{{1
*****************************************************************************/

int AsgnT::ExecLitSet(GsT* pCurLs, int mode) {
    if (pConflictGs) {
        static int AlreadyWarned = false;
        if (AlreadyWarned == false) {
            AlreadyWarned = true;
            fprintf(stderr, "WARNING: ExecLitSet called when conflict exists!\n");
        }
        return -25;
    }
    if (mode > EM_SCHED && pCurLs->id >= pGloProb->NumDoneClauses) {abort();}
    if (pCurLs->TrigRipe()==0) return -40;
    if (HasLit(negv(pCurLs->WatchRawAE[0])) || HasLit(negv(pCurLs->WatchRawAE[1]))) {
        return -50;
    }
    if (USE_PROP_SET) {
        if (WaitingPropSet.count(pCurLs->WatchRawAE[0]) ||
            WaitingPropSet.count(pCurLs->WatchRawAE[1]))
        {
            return -30;
        }
    }
    if (pCurLs->HasBlockedImps()) return -60;
    if (NO_UNIV_REDUC && !HasLit(pCurLs->WatchedRes)) return -65;
    NumExecAttempts++;
    int IsDry = (mode == EM_DRYRUN);
    if (pCurLs->HasAllWatchedTrigs()) {
        if (GetFirstFreeLit(pCurLs->ReqLits) != 0)  {return -70;}
        if (IsDry) return 1;
        if (HasConflict()) {
            return 1;
        }
        pConflictGs = pCurLs;
        assert((pCurLs->InUse & CurPathMask) == 0);
        pCurLs->InUse |= CurPathMask;
        pGloAsgn->FreeFmla = pConflictGs->FreeFmla;
        return 1; 
    } 
    if (mode > EM_SCHED && !PendingExecs[CONFLICT_GST_PRI].is_empty()) {
        pGloAsgn->AddPendingLs(pCurLs, FORC_GST_PRI);
        abort();
        return 0;
    }
    return ExecForced(pCurLs, mode);

}


int AsgnT::ExecForced(GsT* pCurLs, int mode) {

        int IsDry = (mode == EM_DRYRUN);
        
        if ((pCurLs->TrigRipe() == 1)) {
            
            int ForcedLit = negv(pCurLs->WatchLosrAE(0));
            if (GetUpstreamMissingLit(ForcedLit, pCurLs) != 0) {
                return -__LINE__;
            }
            if (1) {
                char qtype = QtypeFromLit(ForcedLit);
                bool OkayFreeFmla;
                switch (qtype) {
                    case 'F': OkayFreeFmla = true; break;
                    case 'E': OkayFreeFmla = (pCurLs->FreeFmla == FmlaFalse); break;
                    case 'A': OkayFreeFmla = (pCurLs->FreeFmla == FmlaTrue); break;
                    default: abort();
                }
                if (!OkayFreeFmla) {
                    fprintf(stderr, "WARNING!  Non-BDD output possible!\n");
                    return -__LINE__;
                }
            }
            if (IsDry) {return ForcedLit;}
            if (QbFromLit[ForcedLit] < MinPropQb) {  // Used in CEGAR simplication.
                return -__LINE__;
            }
            int cnt = 0;
            foreach (itLit, pCurLs->ReqLits) {
                if (!HasLit(*itLit)) cnt++;
            }
            assert(cnt == 1);
            NumProps++;
            pGloAsgn->AppendForcedLit(ForcedLit, pCurLs);
            WaitingPropLits.append(ForcedLit);
            if (USE_PROP_SET) {
                WaitingPropSet.insert(ForcedLit);
            }
            return ForcedLit;

        } else if (pCurLs->TrigRipe() == 2) {
            return 0;
        } else {
            return -490;
        }
}


void AsgnT::SchedForProp(int CurLit) {
    static garr<GsT*> CurLitWatch;  // static for optimization
    assert(HasLit(CurLit));
    assert(CurLitWatch.n == 0);
    CurLitWatch.n = 0;
    foreach (it, pGloAsgn->LitWatch[CurLit]) {
        #if USE_WATCH_SET
        CurLitWatch.append((*it).second);
        #elif USE_WATCH_ARR
        if (*it != NULL) {
            CurLitWatch.append((*it));
        }
        #elif USE_WATCH_LIST
        CurLitWatch.append((*it));
        #endif
    }
    
    int OldN = CurLitWatch.n;
    if (GloDbgLev >= 1800 && CurLitWatch.n != OldN) {
        printf("[%i] ", OldN - CurLitWatch.n);
    }
    for (int ixLs=0; ixLs < CurLitWatch.n; ixLs++) {
        GsT* pCurLs = CurLitWatch[ixLs];
        assert(CurLit > 0);
        FastFixWatch(pCurLs, CurLit);
        ExecLitSet(pCurLs, EM_SCHED);
        if (HasConflict()) break;
    }
    CurLitWatch.n = 0;
}

int AsgnT::Propagate() {
    static garri LitsForced;

    //garr<GsT*> PendingList = garr<GsT*>();

    int first = 0;
    while (true) {
        int flit=0;
        for (int i=first; i < WaitingPropLits.n; i++) {
            flit = WaitingPropLits[i];
            if (flit != 0) {
                WaitingPropLits[i] = 0;
                if (USE_PROP_SET) {
                    WaitingPropSet.erase(flit);
                }
                first = i + 1;
                break;
            } else {
                assert(0);
            }
        }
        if (HasConflict()) {
            if (USE_PROP_SET) {
                WaitingPropSet.erase_vals(WaitingPropLits);
            }
            WaitingPropLits.n = 0;
            break;
        }
        if (flit == 0) {
            if (USE_PROP_SET) {
                WaitingPropSet.erase_vals(WaitingPropLits);
            }
            WaitingPropLits.n = 0;
            return 0;
        }
        SchedForProp(flit);
    }
        
    return 0;
}



/*****************************************************************************
* Stuff for learning a new game state.    }}}{{{1
*****************************************************************************/

bool DumpResols = true;  // For debugging.

struct LitsByChronoIterT {
    set<ChronoT>::iterator it;
    LitT operator*() {return pGloAsgn->ChronoToLit[*it];}
    LitsByChronoIterT& operator--() {--it; return *this;}
    LitsByChronoIterT& operator++() {++it; return *this;}
    bool operator==(LitsByChronoIterT other) {return (it == other.it);}
    bool operator!=(LitsByChronoIterT other) {return (it != other.it);}
    LitsByChronoIterT(set<ChronoT>::iterator _it) : it(_it) { }; 

};

struct LitsByChronoT {
    set<ChronoT> s;
    
    void insert(LitT lit) {s.insert(pGloAsgn->LitToChrono[lit]);}
    void erase(LitsByChronoIterT& it) {s.erase(it.it);}
    size_t size() {return s.size();}
    bool empty() {return s.empty();}
    
    LitsByChronoIterT begin() {return LitsByChronoIterT(s.begin());}
    LitsByChronoIterT end()   {return LitsByChronoIterT(s.end());}
    
};

class TrigHeapT {
    public:
    garr<ChronoT> h;
    void insert(LitT lit) {
        int chrono = pGloAsgn->LitToChrono[lit];
        assert(chrono != NO_DECLEV);
        h.append(chrono);
        push_heap(h.begin(), h.end());
    }

    int pop() {
        assert(h.n > 0);
        ChronoT cret = h[0];
        while (h.n > 0 && h[0] == cret) {
            pop_heap(h.begin(), h.end());
            h.n--;
        }
        return pGloAsgn->ChronoToLit[cret];
    }

    int peek() {
        return pGloAsgn->ChronoToLit[h[0]];
    }

    void dealloc() {
        h.dealloc();
    }

    void dump(const char*);

};

void TrigHeapT::dump(const char* end) {
    garri tmp = h.dup();
    sort(tmp.begin(), tmp.end());
    foreach (it, tmp) {
        *it = pGloAsgn->ChronoToLit[*it];
    }
    DumpLitsEnc(tmp, end);
    tmp.dealloc();
}


class GsBuilder {
    public:
    set<LitT> ImpLits;
    hash_set<LitT> RecTrigLits;
    TrigHeapT TrigHeap;
    garri NonElimLits;  // Used only in minimization
    FmlaT FreeFmla;
    FullStratT* strat;

    void init() {
        TrigHeap.h.init();
        if (StratFile) {
            strat = new FullStratT();
        }
    }

    void dealloc() {
        TrigHeap.dealloc();
    }
    
    void TrigInsert(int lit) {
        assert(pGloAsgn->HasLit(lit));
        if (RecTrigLits.count(lit)) {
            return;
        } else {
            RecTrigLits.insert(lit);
            pGloAsgn->bump_var_act(absv(lit));
            TrigHeap.insert(lit);
        }
    }

    void ImpInsert(int lit) {
        assert(QbFromLit[lit] >= QuantPfx.blks[0].ix);
        //assert(QtypeFromLit(lit) != 'F');
        ImpLits.insert(lit);
    }

    LitT PopTrig() {
        int ret = TrigHeap.pop();
        return ret;
    }

    LitT PeekTrig() {
        int ret = TrigHeap.peek();
        return ret;
    }

    bool TrigEmpty() {
        return TrigHeap.h.n == 0;
    }

    void CopyTrigsTo(garri& TrigLits);

    void dump() {
        printf("{"); 
        this->TrigHeap.dump(", ");
        DumpLitsEnc(this->ImpLits, "} |= ");
        this->FreeFmla->simp_ite()->write(stdout);
        printf("\n"); 
    }
};

void GsBuilder::CopyTrigsTo(garri& TrigLits) {
    int NumLits = TrigLits.n;
    assert(NumLits == TrigHeap.h.n);
    for (int i=0; i < NumLits; i++) {
        TrigLits[i] = pGloAsgn->ChronoToLit[TrigHeap.h[i]];
    }
}

bool AsgnT::HasUIP(GsBuilder& gb) {
    LitT top = gb.TrigHeap.pop();
    LitT penult = gb.TrigHeap.peek();
    gb.TrigHeap.insert(top);
    if (gb.TrigHeap.h.n == 1) return true;
    return (LitToDL[top] != LitToDL[penult]);
}

int AsgnT::GetChronoLastLit(ap<int> Lits) {
    int BestLit=0;
    for (int i=0; i < Lits.n; i++) {
        int CurLit = Lits[i];
        if (LitToChrono[BestLit] < LitToChrono[CurLit]) {
            BestLit = CurLit;
        }
    }
    return BestLit;
}

int AsgnT::GetChronoLastTwoLits(ap<int> Lits, int* p2nd) {
    int BestLit=0, PenLit=0;
    for (int i=0; i < Lits.n; i++) {
        int CurLit = Lits[i];
        if (LitToChrono[BestLit] < LitToChrono[CurLit]) {
            PenLit = BestLit;
            BestLit = CurLit;
        } else if (LitToChrono[PenLit] < LitToChrono[CurLit]) {
            PenLit = CurLit;
        }
    }
    *p2nd = PenLit;
    return BestLit;
}


void AsgnT::PrintDischargeMsg(int CurLit, GsT* pCurLs) {
    const char* qt;
    switch(QtypeFromLit(CurLit)) {
        case 'F': qt = "free(    "; break;
        case 'E': qt = "exists(  "; break;
        case 'A': qt = "forall(  "; break;
        default: abort();
    }
    fprintf(PrfLog, "    %s", qt);
    int n = PrintLitRaw(CurLit, PrfLog);
    fprintf(PrfLog, ",%*s $gs%i)\n", 8 - n, "", pCurLs->id);
}

Fmla* IteSubst(Fmla* test, Fmla* tbra, Fmla* fbra) {
    return ConsFmla(
        FmlaOp::ITE, 
        test, 
        tbra->subst_one(test, fmla_true), 
        fbra->subst_one(test, fmla_false));
}

Fmla* Ite(Fmla* test, Fmla* tbra, Fmla* fbra) {
    return ConsFmla(FmlaOp::ITE, test, tbra, fbra);
}


void AsgnT::Resolve(GsBuilder& gb, int CurLit) {
    assert(!IsChlit(CurLit));
    GsT* pCurLs = GetDepByLit(CurLit);
    assert(pCurLs->InUse);
    assert(pCurLs->ReqLits.arr != NULL);
    pCurLs->bump_activity();
    NumResolves++;
    if (NumResolves > pGloProb->LastPlit) {
        die("Detected an infinite loop in constructing a game-state.\n");
    }
    if (PrfLog) PrintDischargeMsg(CurLit, pCurLs);
    if (StratFile) {
        gb.strat = merge_strats(gb.strat, pCurLs->get_strat(), CurLit);
        if (1 || pCurLs->ImpLits.n > 0) {
            stop();
        }
    }
    int HasSpecial = 0;
    for (int i=0; i < pCurLs->ReqLits.n; i++) {
        int lit = pCurLs->ReqLits[i];
        if (lit == negv(CurLit)) {HasSpecial=lit; continue;}
        gb.TrigInsert(lit);
    }
    assert(HasSpecial);
    FmlaT NewFmla = NULL;
    if (QtypeFromLit(CurLit) == 'F') {
        NewFmla = Ite(int_to_fmla_lit(CurLit), gb.FreeFmla, pCurLs->FreeFmla);
    } else if (QtypeFromLit(CurLit) == 'E') {
        NewFmla = ConsFmla(FmlaOp::OR,  gb.FreeFmla, pCurLs->FreeFmla);
    } else if (QtypeFromLit(CurLit) == 'A') {
        NewFmla = ConsFmla(FmlaOp::AND, gb.FreeFmla, pCurLs->FreeFmla);
    } else {
        abort();
    }
    // if (QtypeFromLit(CurLit) != 'F' && (gb.FreeFmla != pCurLs->FreeFmla || gb.FreeFmla != PlyFmlaFromLit(CurLit)->negate()))
    if (QtypeFromLit(CurLit) == 'F' || (gb.FreeFmla != pCurLs->FreeFmla || gb.FreeFmla != PlyFmlaFromLit(CurLit)->negate())) 
    {
        assert(AllowFree);
        gb.ImpInsert(CurLit);
        gb.ImpInsert(negv(CurLit));
    }
    gb.FreeFmla = NewFmla;
    for (int i=0; i < pCurLs->ImpLits.n; i++) {
        int CurImp = pCurLs->ImpLits[i];
        gb.ImpInsert(CurImp);
    }
    if (GloDbgLev >= 2020) {
        printf("Resolved ");
        DumpLitRaw(CurLit);
        printf(" via GS[%i]: ", pCurLs->id);
        printf("{"); 
        DumpLitsEnc(pCurLs->ReqLits, ", ");
        DumpLitsEnc(pCurLs->ImpLits, "} |= ");
        pCurLs->FreeFmla->simp_ite()->write(stdout);
        printf("\nNow: ");
        gb.dump();
    }
}

int NumRedunCacheHits;
int GloUIP; // For debugging.


/*****************************************************************************
* MakeLearnedGs: Analyze conflict and learns a new game state.    }}}{{{1
*****************************************************************************/
int BadImp = 0;  // for debugging
GsT* AsgnT::MakeLearnedGs() {
    GsBuilder gb = GsBuilder();
    gb.init();
    NumResolves = 0;
    assert(pGloAsgn->HasConflict());

    pConflictGs->bump_activity();
    GsT* pInitGs = pConflictGs;
    if (PrfLog) {
        fprintf(PrfLog, "$gs%i:resolve($gs%i, [\n", *pNumClauses, pInitGs->id);
    }
    {
        assert(HasConflict());
        assert(pInitGs != NULL);
        assert(pInitGs->HasAllWatchedTrigs());
        LitsInConflict.insert(GetChronoLastLit(pInitGs->ReqLits));
        gb.FreeFmla = pInitGs->FreeFmla;
        foreach (it, pInitGs->ReqLits) {
            gb.TrigInsert(*it);
        }
        foreach (it, pInitGs->ImpLits) {
            gb.ImpInsert(*it);
        }
        gb.strat = pInitGs->get_strat();
    }

    if (DumpResols && GloDbgLev >= 2010) {
        if (GloDbgLev >= 2015) {printf("Lits: "); DumpLits(0);}
        //DumpLitsEnc(gb.ConflictAsgn, "\n");
    }

    uint CurDL = GetCurDL();
    int NumCur = 0;
    foreach (itLit, pConflictGs->ReqLits) {
        if (LitToDL[*itLit] == CurDL) {
            NumCur++;
        }
    }
    
    if (GloDbgLev >= 2020) {
        printf("\nNow: ");
        gb.dump();
    }

    int CurLit=0;
    int winner = GetWinner();
    GsT* pLrn = NULL;
    while (true) {
        if (gb.TrigEmpty()) break;
        CurLit = gb.PeekTrig();
        bool ok = false;
        if (CurDL > 0) {
            int QtypeCurDL = GloQtype(QbFromDL(LitToDL[CurLit]));
            int QtypeCurLit = QtypeFromLit(CurLit);
            if (winner != 'F') {
                assert(winner == 'E' || winner == 'A');
                if (!AllowFree) {assert(QtypeCurLit != winner);}
                ok = (QtypeCurLit != winner && QtypeCurDL != winner) && HasUIP(gb);
            } else {
                assert(QtypeCurDL == 'F');
                ok = HasUIP(gb);
            }
        }
        if (ok) {
            BadImp = 0;
            foreach (itImp, gb.ImpLits) {
                if (QbFromLit[CurLit] <= QbFromLit[*itImp]) continue;
                if (LitToDL[CurLit] <= LitToDL[*itImp]) {
                    BadImp = *itImp;
                    goto TryAgainUIP;
                }
            }
            if (IsChlit(CurLit)) {
                break;
            }
            if (LitsInConflict.count(CurLit)==0) {
                pLrn = CreateGsFromGb(gb);
                pLrn->LrnType = LRN_TYPE_2;
                GloUIP = gb.TrigHeap.peek();
                break;
            }
            TryAgainUIP:;
        }
        gb.PopTrig();
        Resolve(gb, CurLit);
    }
    if (PrfLog) {
        fprintf(PrfLog, "# UIP: ");
        PrintLitRaw(CurLit, PrfLog);
        fprintf(PrfLog, "\n");
        fprintf(PrfLog, "])\n");
    }

    if (pLrn == NULL) {
        pLrn = CreateGsFromGb(gb);
        pLrn->LrnType = LRN_TYPE_1;
        GloUIP = gb.TrigHeap.peek();
    }
    if (0) {pLrn->dump();}
    gb.dealloc();
    return pLrn;

}




/*****************************************************************************
* More stuff related to learning.    }}}{{{1
*****************************************************************************/

#define REDUN_MODE_1 1
#define REDUN_MODE_2 2

inline bool AsgnT::IsRedundantBySeq(int CurLit, GsBuilder& gb, GsT* pDepLs, int mode) {
    bool HasForced = false;
    int ForcedQb = QbFromLit[CurLit];
    foreach (it, pDepLs->ReqLits) {
        if (*it == negv(CurLit)) {HasForced = true; continue;}
        int ChronoInv = (LitToChrono[CurLit] < LitToChrono[*it]);
        if (mode == REDUN_MODE_1) {
            if (gb.RecTrigLits.count(*it) == 0) {return false;}
            if (ChronoInv) {return false;}
        } else if (mode == REDUN_MODE_2) {
            if (gb.NonElimLits.bin_search(*it) == false) {return false;}
            if (ForcedQb < QbFromLit[*it]) {return false;}
            if (ForcedQb == QbFromLit[*it] && ChronoInv) {return false;}
        }
    }
    assert(HasForced);
    foreach (it, pDepLs->ImpLits) {
        if (gb.ImpLits.count(*it) == 0) {
            return false;
        }
    }
    if (GloDbgLev >= 2001) 
        printf("Removed %i via pGloProb->clauses.arr[%i]\n", CurLit, pDepLs->id);
    return true;
}

bool AsgnT::IsRedundant(int CurLit, GsBuilder& gb, int mode) {
    if (QtypeFromLit(CurLit) == 'F') return false;
    if (LitToDL[CurLit] == 0) return true;
    GsT* pDepLs = GetDepByLit(CurLit);
    if (pDepLs == NULL) {
        assert(IsChlit(CurLit));
        return false;
    }
    if (IsRedundantBySeq(CurLit, gb, pDepLs, mode)) return true;
    if (mode == REDUN_MODE_2) {
        foreach (it, pGloAsgn->LitWatch[negv(CurLit)]) {
            GsT* pCurLs = *it;
            if (pCurLs == NULL) continue;
            if (IsRedundantBySeq(CurLit, gb, pCurLs, mode)) {
                return true;
            }
        }
    }
    return false;
}

void AsgnT::MinimizeConflict(GsBuilder& gb, garri& TrigLits) {
    /* Sorensson & Biere, "Minimizing Learned Clauses", SAT 2009. */
    map<LitT, bool> cache;
    //garri orig = TrigLits.dup();  // For debugging.
    for (int i=0; i < TrigLits.n; i++) {
        int CurLit = TrigLits[i];
        if (IsRedundant(CurLit, gb, REDUN_MODE_1)) {
            TrigLits[i] = 0;
        }
    }
    int NumOrig = TrigLits.n;
    TrigLits.remove_value(0);
    int delta = NumOrig - TrigLits.n;
    if (delta) {
        //printf("Removed %i\n", delta);
    }
    //orig.dealloc();
}


void GsT::InitWatchedLitsPart1() {
    GsT* pCurLs = this;
    assert(LitToDL[0] == 0);
    int WatchW[2] = {0,0};
    for (int i=0; i < pCurLs->ReqLits.n; i++) {
        int wlit = pCurLs->ReqLits[i];
        if (LitToDL[wlit] > LitToDL[WatchW[0]]) {
            WatchW[1] = WatchW[0];
            WatchW[0] = wlit;
        } else
        if (LitToDL[wlit] > LitToDL[WatchW[1]]) {
            WatchW[1] = wlit;
        }
    }
    if (pCurLs->ImpLits.n > 0) {
        int BestFut = pCurLs->ImpLits[0];
        foreach (itFut, pCurLs->ImpLits) {
            if (LitToDL[*itFut] > LitToDL[BestFut]) {
                BestFut = *itFut;
            }
        }
        pCurLs->WatchedRes = BestFut;
    }
    pCurLs->WatOrd.alloc(max(2, pCurLs->ReqLits.n));
    for (int i=0; i < ReqLits.n; i++) {
        WatOrd[i] = ReqLits[i];
    }
    pCurLs->WatchRawAE[0] = WatchW[0];
    pCurLs->WatchRawAE[1] = WatchW[1];
    if (pCurLs->WatchLosrAE(0) != 0) assert(pCurLs->WatchLosrAE(0) != pCurLs->WatchLosrAE(1));
}


GsT* AsgnT::CreateGsFromGb(GsBuilder& gb) {
    GsT* pCurLs = talloc(GsT, 1);
    pCurLs->init();

    int NumLits = gb.TrigHeap.h.n;
    garri TrigLits = garri(NumLits);
    gb.CopyTrigsTo(TrigLits);
    if (!AllowFree) MinimizeConflict(gb, TrigLits);
    timsort(TrigLits);
    TrigLits.remove_adj_dups();
    int MaxLosrQb = 0;
    foreach (itLit, TrigLits) {
        MaxLosrQb = max(MaxLosrQb, (int)QbFromLit[*itLit]);
    }
    if (MaxLosrQb == 0) {
        MaxLosrQb = MAX_QB;
    }

    int NumImps = gb.ImpLits.size();
    pCurLs->ImpLits.alloc(NumImps);
    {
    int i = 0;
    foreach (itLit, gb.ImpLits) {
        if (MaxLosrQb < QbFromLit[*itLit]) {continue;}
        assert(i < pCurLs->ImpLits.n);
        pCurLs->ImpLits[i++] = *itLit;
    }
    if (pCurLs->ImpLits.n != i) {
        NumReducs++;
    }
    pCurLs->ImpLits.n = i;
    }
    pCurLs->ReqLits = dup_as_ap(TrigLits);
    TrigLits.dealloc();


    pCurLs->FreeFmla = gb.FreeFmla;
    //assert(FreeFmla==FmlaTrue || FreeFmla==FmlaFalse);

    timsort(pCurLs->ReqLits);
    pCurLs->InitTrigLits(pCurLs->ReqLits);
    pCurLs->InitWatchedLitsPart1();

    if (NumLits == 0) {
        AssignLrnId(pCurLs);
        pCurLs->set_strat(gb.strat);
        return pCurLs;
    }
    
    int BestImp = 0;
    int LastTrig = pCurLs->WatchLosrAE(0);
    int Penult  =  pCurLs->WatchLosrAE(1);
    if (!gb.ImpLits.empty()) {BestImp = *gb.ImpLits.begin();}
    foreach (itLit, gb.ImpLits) {
        if (QbFromLit[*itLit] > QbFromLit[LastTrig]) continue;
        if (LitToDL[*itLit] > LitToDL[Penult]) {
            if (LitToDL[*itLit] > LitToDL[BestImp]) {
                BestImp = *itLit;
            }
        }
    }
    pCurLs->WatchedRes = BestImp;
    if (pCurLs->ImpLits.n > 0) assert(pCurLs->WatchedRes != 0);

    if (GloSanChk >= 2) {
        vector<LitT>* pRawInts = pCurLs->AsLitVec();
        auto(itOld, DupLrnTest.find(*pRawInts));
        if (itOld != DupLrnTest.end()) {
            GsT* pOld = itOld->second;
            pOld->PrintIndex();
            printf("THIS SHOULDN'T BE HAPPENING\n");
            assert(0);
        }
        DupLrnTest[*pRawInts] = pCurLs;
        delete pRawInts;
    }
    pCurLs->IsLearned = true;
    pCurLs->LastUse = NumBigBt;
    pCurLs->bump_activity();

    // if (pCurLs->FreeFmla != FmlaTrue && pCurLs->FreeFmla != FmlaFalse) {
    //     for (int i=0; i < pCurLs->ReqLits.n; i++) {
    //         int lit = pCurLs->ReqLits[i];
    //         if (QtypeFromLit(lit) != 'F') {
    //             stop();
    //         }
    //     }
    // }

    AssignLrnId(pCurLs);
    pCurLs->set_strat(gb.strat);
    return pCurLs;
}


void AsgnT::AssignLrnId(GsT* pCurLs) {
    pCurLs->id = (pGloProb->clauses.n);
    pGloProb->clauses.append(pCurLs);
    if (pCurLs->ReqLits.n > 3) {
        TempLearntClauses.insert(pCurLs);
    }
}


void AsgnT::AddNewLitSet(GsT* pCurLs) {
    pCurLs->RegisterLitsHave();
    int* NewWatchesB = pCurLs->WatchRawArr;
    for (int i=0; i < 2; i++) {
        if (NewWatchesB[i] == 0) continue;
        pCurLs->itWatchRawAE[i] = pGloAsgn->LitWatchInsert(pCurLs->WatchRawAE[i], pCurLs);
    }
    pCurLs->itWatchRes = pGloAsgn->LitWatchInsert(pCurLs->WatchedRes, pCurLs);

}


void AsgnT::PrintNewLitSet(GsT* pCurLs, const char* extra, int DeltaBT) {
    if (GloDbgLev >= 1900) {
        printf("%s Lrn GS #%i (L%i). ", extra, pCurLs->id, pCurLs->LrnType);
    }
    if (GloDbgLev >= 1800) {
        // char WinrLetr = '-';
        // if (pCurLs->FreeFmla == FmlaTrue) {WinrLetr = 'E';}
        // if (pCurLs->FreeFmla == FmlaFalse) {WinrLetr = 'A';}
        printf("(");
        int DeltaQB = QbFromLit[LastChoice] - QbFromLit[GetLastChlit()];
        printf("QB %2i%c, ", QbFromLit[LastChoice], tolower(QtypeFromLit(LastChoice)));
        if (DeltaBT) printf(UTF8_DELTA "DL %4i, " UTF8_DELTA "QB %2i, %3i resolvs, ", 
            DeltaBT, DeltaQB, NumResolves);
        //printf("%4i%c, ", LastChoice, tolower(QtypeFromLit(LastChoice)));
        if (!DeltaBT) printf("                  ");
        printf("): ");
        pCurLs->FreeFmla->simp_ite()->write(stdout);
    }
    if (GloDbgLev >= 2000) {
        printf("; Lnow=");
        DumpLitsEnc(pCurLs->ReqLits, "");
        printf("; Lfut=");
        DumpLitsEnc(pCurLs->ImpLits, "");
    }
    if (GloDbgLev >= 1950 && GloDbgLev < 2000) {
        printf("; Lnow=");
        DumpLitsEnc(pCurLs->ReqLits, "");
    }
    if (1900 <= GloDbgLev && GloDbgLev < 1950) {
        if (pCurLs->ReqLits.n <= 1) {
            DumpLitsEnc(pCurLs->ReqLits, "");
        } else {
            int NumLits = pCurLs->ReqLits.n;
            printf("   ReqLits.n = %i ", NumLits);
        }
    }
    if (GloDbgLev >= 1800) {
        const char* sep;
        if (GloDbgLev >= 2000) {sep = "\n\n"; NeedNewl=false;}
        else if (GloDbgLev >= 1900) {sep = "\n"; NeedNewl=false;}
        else if (NumBigBt % 2 == 0) {sep = "\n"; NeedNewl=false;}
        else {sep = ""; NeedNewl = true;}
        printf("%s", sep);
    }

}


GsT* pOldConflict = NULL;  // For debugging.

/*****************************************************************************
* LearnAndBacktrack.    }}}{{{1
*****************************************************************************/
GsT* AsgnT::LearnAndBacktrack(int GotTwo) {
    int WinrQtype = QTYPE_BAD;
    if (0) {}
    else if (FreeFmla == FmlaTrue)  {WinrQtype = QTYPE_E1;}
    else if (FreeFmla == FmlaFalse) {WinrQtype = QTYPE_A0;}
    else {assert(AllowCegar == 0);}
    int LastChlit = GetLastChlit();
    int ElimQb = QbFromLit[LastChlit];
    pOldConflict = pConflictGs;
    bool DoCegar = (
        HasMoreVars() &&
        (AllowCegar >= 1) && LastChlit != 0 && 
        !pGloProb->NoMoreVars && 
        true);
    if (DoCegar) {
        if (WinrQtype == QtypeNumFromLit(LastChlit)) {
            DoCegar = true;
        } else {
            DoCegar = 
            (LitToDL[pConflictGs->WatchRawAE[0]] != LitToDL[pConflictGs->WatchRawAE[1]] &&
             QbFromLit[pConflictGs->WatchedRes] < QbFromLit[pConflictGs->WatchRawAE[0]]);
            if (DoCegar) {
                ElimQb = QbFromLit[pConflictGs->WatchedRes];
            }
        }
        if (ElimQb <= 1) {
            DoCegar = false;
        }
        if (ElimQb < InnermostInputQb - 3) {   // TODO: Consider removing this.
            DoCegar = false;
        }
    }
    
    static garri AsgnLits = garri(0,0);
    AsgnLits.n = 0;

    GsT* pLrn = pGloAsgn->MakeLearnedGs();
    // if (0) {
    //     map<Fmla*,Fmla*> rep = map<Fmla*,Fmla*>();
    //     map<Fmla*,Fmla*> cache;
    //     cache = map<Fmla*,Fmla*>();
    //     foreach (itLit, pLrn->ReqLits) {
    //         rep[gate_to_fmla[absv(*itLit)]] = IsPosLit(*itLit) ? fmla_true : fmla_false;
    //     }
    //     foreach (itLit, pLrn->ImpLits) {
    //         rep[gate_to_fmla[absv(*itLit)]] = IsPosLit(*itLit) ? fmla_true : fmla_false;
    //     }
    //     Fmla* orig_subst = OrigFmla->subst(rep, cache);
    //     cache = map<Fmla*,Fmla*>();
    //     Fmla* free_subst = pLrn->FreeFmla->subst(rep, cache);
    //     assert(orig_subst->to_bdd() == free_subst->to_bdd());
    // }
    AddNewLitSet(pLrn);
    if (pLrn->ReqLits.n == 0) {
        PrintNewLitSet(pLrn, "$", 0);
        return pLrn;
    }

    uint InitDL = GetCurDL();

    InitChoices.n = 0;

    unsigned int TargDL = LitToDL[pLrn->WatchRawAE[1]];
    if ((QbFromLit[pLrn->WatchedRes] < QbFromLit[pLrn->WatchRawAE[0]]) || NO_UNIV_REDUC) {
        TargDL = max(TargDL, LitToDL[pLrn->WatchedRes]);
    }


    /* Make AsgnLits */
    if (DoCegar) {
        foreach (itLit, pLrn->ImpLits) {
            if (!((QtypeNumFromLit(*itLit)==WinrQtype) && IsInputLit(*itLit))) {
                continue;
            }
            if (QbFromLit[*itLit] != ElimQb) continue;
            AsgnLits.append(*itLit);
        }
        
        foreach (itChlit, chlits.flat) {
            garri& CurUndo = UndoListByDL[LitToDL[*itChlit]];
            foreach (itLit, CurUndo) {
                int lit = *itLit;
                if (lit == 0) continue;
                if (!IsInputLit(lit)) {continue;}
                //if (QtypeNumFromLit(lit) != WinrQtype) {continue;}
                //if (QbFromLit[lit] != ElimQb) continue;
                if (QbFromLit[lit] == ElimQb - 1) continue;
                //if (QbFromDL(TargDL) <= QbFromLit[lit] && QbFromLit[lit] < ElimQb) continue;
                if (ElimQb < QbFromLit[lit]) continue;
                if (lit > pGloProb->RawLastInpLit) continue;
                AsgnLits.append(lit);
            }
        }
    
        timsort(AsgnLits);
        AsgnLits.remove_adj_dups();
        LitT prev = 0;
        foreach (itLit, AsgnLits) {
            if (*itLit == negv(prev)) {
                DoCegar = false;
                if (GloDbgLev >= 1100) {
                    printf("Cegar aborted due to contradictory winner lits (QB %i, contra lit %i)\n",
                        ElimQb, absv(prev));
                    break;
                }
            }
            prev = *itLit;
        }
    }


    if (1) {
        while (GetCurDL() != TargDL) {
            pGloAsgn->Backtrack();
        }
        int ret = ExecLitSet(pLrn, EM_SCHED); assert(ret > 0);
        pGloAsgn->Propagate();
    } /* else {
        while (true) {
            if (GetLastChlit() == ZERO_LIT) break;
            int PrevLit = pGloAsgn->Backtrack();
            if (pConflictGs != NULL) {continue;}
            bool IsBlocked = pLrn->HasBlockedImps();
            bool IsAsserting = (pLrn->HasAllWatchedTrigs() || ExecLitSet(pLrn, EM_DRYRUN) > 0);
            if (IsBlocked) continue;
            if ((IsAsserting && GetCurDL() > 0)) {
                continue;
            } else {
                pGloProb->NumDoneClauses = pGloProb->clauses.n;
                int ret = ExecLitSet(pLrn, EM_SCHED); 
                if (ret > 0) {
                    pGloAsgn->Propagate();
                } else {
                    AppendChlit(PrevLit);
                    SchedForProp(PrevLit);
                    pGloAsgn->Propagate();
                }
                break;
            }
        }
    } */
    assert(GetCurDL() == TargDL);
    int DeltaBT = InitDL - GetCurDL();
    assert(DeltaBT >= 0);
    PrintNewLitSet(pLrn, (GotTwo ? "*" : "+"), DeltaBT);
    /* if (0) {
        for (int ixCl = LastOrigClauseID; ixCl < pGloProb->clauses.n; ixCl++) {
            GsT* pTestCl = pGloProb->clauses[ixCl];
            if (pTestCl == NULL) continue;
            if (pTestCl == pLrn) continue;
            if (pTestCl->is_subsumed_by(pLrn)) {
                if (GloDbgLev >= 100) {
                    printf(" %5i subsumed by %5i\n", pTestCl->id, pLrn->id);
                }
                //pTestCl->dealloc();
            }
        }
    } */
    NumBigBt++;
    
    if (DeltaBT == 0) {
        abort();
        //restart();
    }

    if (GloSanChk >= 3 && !HasConflict()) WatchSanityChk3();
    if (GloSanChk >= 4 && !HasConflict()) LatentExecsChk();

    if (DoCegar) {
        map<int,int> *hit = new map<int,int>();

        garri OldChlits = chlits.flat.dup();
        restart();   // TODO: Don't restart!

        pGloProb->NumDoneClauses = pGloProb->clauses.n;
        GsT* CegarGs = NULL;
        int OldNumVars = pGloProb->NextNewVar;
        if (AllowCegar >= 1) {
            try {
                CegarGs = Augment(pGloProb->OutGlit, AsgnLits, hit, ElimQb);
            } catch (char const* s) {
                static int AlreadyWarned = false;
                pGloProb->NoMoreVars = true;
                if (AlreadyWarned == false) {
                    AlreadyWarned = true;
                    fprintf(stderr, "\nRan out space for new variables!\n");
                }
            }
        }
        if (CegarGs) {
            if (GloDbgLev >= 1100) {
                int NumNewVars = (pGloProb->NextNewVar - OldNumVars)/2;
                printf("CEGAR %i: QB=%i, NumNewVars=%i, NumAsgnVars=%i\n", 
                    NumCegLrn, ElimQb, NumNewVars, AsgnLits.n);
            }
        }
        delete hit;

        if (CegarGs) {
            NumCegLrn++;
            ExecLitSet(CegarGs, EM_SCHED);
        } else if (AllowCegar >= 1) {
            static int AlreadyWarned = false;
            if (AlreadyWarned == false) {
                AlreadyWarned = true;
                if (!pGloProb->NoMoreVars) {
                    fprintf(stderr, "WARNING: CEGAR failed!\n");
                } else if (GloDbgLev >= 100) {
                    fprintf(stderr, "(Out of variables, no more CEGAR.)\n");
                }
            }
        }
        Propagate();

        if (GloSanChk >= 4 && !pGloAsgn->HasConflict()) LatentExecsChk();
        foreach (itChlit, OldChlits) {
            LitT chlit = *itChlit;
            if (HasConflict()) {break;}
            if (HasLit(negv(chlit))) {break;}
            if (HasLit(chlit)) {continue;}
            AppendChlit(chlit);
            SchedForProp(chlit);
            Propagate();
        }
        OldChlits.dealloc();
        if (GloSanChk >= 3 && !pGloAsgn->HasConflict()) WatchSanityChk3();
        if (GloSanChk >= 4 && !pGloAsgn->HasConflict()) LatentExecsChk();
        
    }

    return NULL;
}


void AsgnT::PreSolve() {
    LastDefnGs = pGloProb->clauses.n - 1;
    if (!NoUnivGhost) {
        NewLrnGs_Top(ghosted(pGloProb->OutGlit, 0), 1);
        if (HasConflict()) {return;}
        NewLrnGs_Top(ghosted(negv(pGloProb->OutGlit), 1), 0);
        if (HasConflict()) {return;}
    }
    LastInitGs = pGloProb->clauses.n - 1;

    pGloProb->NumDoneClauses = pGloProb->clauses.n;
    pGloAsgn->Propagate();
    foreach (it, pGloProb->clauses) {
        if (*it == NULL) continue;
        ExecLitSet(*it, EM_SCHED);
        pGloAsgn->Propagate();
        if (HasConflict()) {return;}
    }
    WatchSanityChk3();
    if (!pGloAsgn->HasConflict()) LatentExecsChk();

    pGloAsgn->Propagate();
    WatchSanityChk3();

    ixChoi = 0;
    NumSinceCut = 0;
    NumUntilRestart = 2;
    LastPrintTime = GetUserTimeMicro();
    NumRestarts = 0;

    if (0) {
        int ElimQb = 2;
        assert(pGloProb->QuantBlk.n > ElimQb);
        int n = 0;
        foreach (itLit, pGloProb->QuantBlk[ElimQb].lits) {
            if (IsInputLit(*itLit)) {n++;}
        }
        for (int i=0; i < (1 << n); i++) {
            garri AsgnLits = garri(n);
            for (int j=0; j < n; j++) {
                AsgnLits[j] = pGloProb->QuantBlk[ElimQb].lits[j] ^ ((i >> j) % 2);
            }
            DumpLitsEnc(AsgnLits);
            map<int,int> *hit = new map<int,int>();
            if (1) {
                Augment(pGloProb->OutGlit, AsgnLits, hit, ElimQb);
            }
            if (0) {
                CegarExtend(AsgnLits, hit, ElimQb);
            }
            delete hit;
        }
        Propagate();
    }

    clean_up_sequents();
    
}


int AsgnT::choose_lit_and_prop(int choice) {
    int NewLit = pGloAsgn->ChooseRandLit(choice);
    if (GloDbgLev >= 2002) {
        printf("(pick %i) ", NewLit);
    }
    LastChoice = NewLit;
    AppendChlit(NewLit);
    pGloAsgn->SchedForProp(NewLit);
    pGloAsgn->Propagate();
    return NewLit;
}

template <typename T>
class CircBufr {
    public:
    int size;
    int cur_ix;
    T* buffer;

    CircBufr(int arg_size) {
        size = arg_size;
        buffer = talloc(T, size);
        cur_ix = 0;
    }
    ~CircBufr() {
        free(buffer);
        buffer = NULL;
        size = 0;
    }
    T& prev_val() {
        return buffer[(cur_ix - 1 + size) % size];
    }
    T& cur_val() {
        return buffer[cur_ix];
    }
    void advance() {
        cur_ix = (cur_ix + 1) % size;
    }

};


/*****************************************************************************
* Top-level solver function.    }}}{{{1
*****************************************************************************/
GsT* AsgnT::solve() {

    long long LastPrintBt = 0;
    long long LastPrintProps = 0;
    auto(AgeCutoff, CircBufr<int>(4));
    while (1) {
        while (!pGloAsgn->HasConflict()) {
            int choice = 0;
            while (ixChoi < InitChoices.n) {
                choice = InitChoices[ixChoi];
                if (pGloAsgn->HasLit(choice)) {ixChoi++; continue;}
                if (pGloAsgn->HasLit(negv(choice))) {
                    InitChoices.n = 0;
                }
                break;
            }
            if (GloSanChk >= 5 && !pGloAsgn->HasConflict()) LatentExecsChk();
            choose_lit_and_prop(choice) || die_f("\nERROR: NO CHOICES!  (NumBt = %i)\n", NumBigBt);
            GloStep++;
        }
        int GotOne = false;
        int GotTwo = false;
        GsT* pFinal;
        //if (GloSanChk >= 3) WatchSanityChk3();
        while (pGloAsgn->HasConflict()) {
            if (GotOne) GotTwo = true;
            GotOne = true;

            pFinal = LearnAndBacktrack(GotTwo);
            cla_decay_activity();
            var_decay_activity();
            LitHitInc *= 1.01;


            if (--learntsize_adjust_cnt == 0){
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt    = (int)learntsize_adjust_confl;
                max_learnts             *= learntsize_inc;
            }


            if (pFinal != NULL) {
                if (pFinal->ReqLits.n == 0) {
                    return pFinal;
                }
            }

            delete_excess_learned_clauses();

            NumUntilRestart--;
            NumSinceCut++;
            if (GloTimeOut != 0  &&  (GetUserTimeMicro() - StartTime) > GloTimeOut) {
                if (!QuietMode) {
                    PrintStats();
                    fprintf(stderr, "TimeOut!  (%d backtracks)\n", NumBigBt);
                } else {
                    fprintf(stdout, "TimeOut.\n");
                }
                exit(0);
            }
        }

        if (GotOne) {
            /* Random restarts. */
            if (NumUntilRestart <= 0 || !NoRestart) {
                if (GloDbgLev >= 100) {
                    long long CurTime = GetUserTimeMicro();
                    int DeltaTime = (CurTime - LastPrintTime);
                    if (GloDbgLev >= 3000 || DeltaTime >= 500*1000) {
                        LastPrintTime = CurTime;
                        if (NeedNewl) {printf("\n"); NeedNewl=false;}
                        printf("### RESTART %3i ###  ", NumRestarts);
                        printf("(Bt %5i, C %2i, speed:%6.0f conflicts/sec, %7.0f props/sec)\n",
                            NumBigBt, NumCegLrn,
                            ((double)(NumBigBt - LastPrintBt) / (double)DeltaTime * 1000000),
                            ((double)(NumProps - LastPrintProps) / (double)DeltaTime * 1000000)
                            );
                        LastPrintBt = NumBigBt;
                        LastPrintProps = NumProps;
                    }
                }
            }
            if (NumUntilRestart <= 0 && !NoRestart) {
                NumUntilRestart = RestartCycle;
                NumRestarts++;
                if (NumRestarts <= 4) {
                    NumUntilRestart = 8;
                } else {
                    RestartCycle += (int) sqrt(RestartCycle / 16);
                }
                if (GloDbgLev >= 2000) printf("\n");
                if (1 || NumSinceCut >= 8) {
                    NumSinceCut = 0;
                }
                AgeCutoff.cur_val() = NumBigBt;
                AgeCutoff.advance();
                pGloAsgn->restart();
                CurPathNum = (CurPathNum + 1) % (MaxPathNum + 1);
                CurPathMask = PathMaskFromNum(CurPathNum);
                if (CurPathNum == PathNum1) {
                    RandomizePolPref();
                }
                InitChoices.n = 0;
            }
        }
    }
    assert(false);
}



/****************************************************************************/


void AsgnT::NewDefnGsAlt(LitT ixExpr, garri args) {
    static bool WarnDuplicate = true;
    args = args.dup();
    timsort(args);
    args.remove_adj_dups();
    auto(entry, HashedApInt(args));
    if (ArgsToGlit.count(entry)) {
        int old = ArgsToGlit[entry];
        if (WarnDuplicate && GloDbgLev >= 20) {
            WarnDuplicate = false;
            fprintf(stderr, "Redundant gate: %i has same inputs as %i.\n", ixExpr, old);
        }
    } else {
        ArgsToGlit[entry] = ixExpr;
    }

    garri lits = garri(0, args.n + 1);
    for (int ply=0; ply <= 1; ply++) {
        if (ply == 0 and NoUnivGhost) continue;
        lits.n = 0;
        int GhostLit = ghosted(negv(ixExpr), ply);
        lits.append(GhostLit);
        for (int i=0; i < args.n; i++) {
            lits.append(ghosted_or_pass(args[i], ply));
        }
        GsT* pCurLs = NewLrnGs_Mixed(lits, 1 - ply);
        pCurLs->GlitDefd = GhostLit;
        GhostToGs[ghosted(ixExpr, ply)] = pCurLs;
        GhostToGs[negv(ghosted(ixExpr, ply))] = NON_INPUT;
        if (IsInCegar) TempNewClauses.append(pCurLs);
    }
    for (int i=0; i < args.n; i++) {
        for (int ply=0; ply <= 1; ply++) {
            if (ply == 0 and NoUnivGhost) continue;
            lits.n = 0;
            int GhostGate = ghosted(ixExpr, ply);
            lits.append(ghosted_or_pass(negv(args[i]), ply));
            lits.append(GhostGate);
            GsT* pCurLs = NewLrnGs_Mixed(lits, 1 - ply);
            pCurLs->GlitDefd = GhostGate;
            if (IsInCegar) TempNewClauses.append(pCurLs);
        }
    }
    lits.dealloc();
    args.dealloc();
    return;
}


GsT* AsgnT::NewLrnGs_Top(LitT top, int winr) {
    garri args = garri(1, 1);
    args[0] = top;
    GsT* ret = NewLrnGs_Mixed(args, winr);
    args.dealloc();
    return ret;
}

GsT* AsgnT::NewLrnGs_Mixed(garri AsgnLits, int winr) {
    static garri args;
    static garri WinrLits;
    args.n = 0;
    WinrLits.n = 0;
    foreach (itLit, AsgnLits) {
        char QtypeLetr = QtypeFromLit(*itLit);
        if (QtypeLetr != 'F' && QtypeNumFromLetr(QtypeLetr) == winr) {
            WinrLits.append(*itLit);
        } else {
            args.append(*itLit);
        }
    }
    return NewLrnGs(args, WinrLits, winr ? FmlaTrue : FmlaFalse);
}

GsT* AsgnT::NewLrnGs(garri args, garri WinrLits, FmlaT winr) {
    assert(winr==FmlaTrue || winr==FmlaFalse);
    GsT* pCurLs = talloc(GsT, 1);
    pCurLs->id = pGloProb->clauses.n;
    pGloProb->clauses.append(pCurLs);
    pCurLs->init();
    pCurLs->FreeFmla = winr;
    {
        garri tmp = args.dup();
        timsort(tmp);
        //tmp.sort(CmpLitByPoit);
        tmp.remove_adj_dups();
        pCurLs->ReqLits = dup_as_ap(tmp);
        tmp.dealloc();
        pCurLs->InitTrigLits(pCurLs->ReqLits);
        pCurLs->RegisterLitsHave();
    }
    {
        garri tmp = WinrLits.dup();
        timsort(tmp);
        //tmp.sort(CmpLitByPoit);
        tmp.remove_adj_dups();
        pCurLs->ImpLits = dup_as_ap(tmp);
        tmp.dealloc();
    }
    if (StratFile) {
        FullStratT* strat = strat_from_Lfut(pCurLs->ImpLits);
        pGloProb->clause_id_to_strat.setval(pCurLs->id, strat);
        //if (pCurLs->ImpLits.n > 0) {
        //    stop();
        //}
    }
    if (pCurLs->ImpLits.n > 0) {
        //stop();
    }
    pCurLs->InitWatchedLitsPart1();
    //assert(pCurLs->id != 42);
    AddNewLitSet(pCurLs);
    if (PrfLog) {
        fprintf(PrfLog, "$gs%i:gseq(\n", pCurLs->id);
        fprintf(PrfLog, "    [");
        foreach (it, pCurLs->ReqLits) {PrintLitRaw(*it, PrfLog); fprintf(PrfLog, ", ");}
        fprintf(PrfLog, "],\n    [");
        foreach (it, pCurLs->ImpLits) {PrintLitRaw(*it, PrfLog); fprintf(PrfLog, ", ");}
        fprintf(PrfLog, "],\n    ");
        pCurLs->FreeFmla->write(PrfLog);
        fprintf(PrfLog, ")\n");
    }
    pGloProb->NumDoneClauses = pGloProb->clauses.n;
    ExecLitSet(pCurLs, EM_SCHED);
    Propagate();
    return pCurLs;
}


/****************************************************************************/
void add_var_to_qblock(int qvar, int ixQb) {
    pGloProb->QuantBlk[ixQb].lits.append(qvar);
    assert(qvar == absv(qvar));
    assert(QbFromLit[qvar] == 0);
    QbFromLit[qvar] = ixQb;
    QbFromLit[negv(qvar)] = ixQb;
    pGloAsgn->InsertVarActOrder(qvar);
}

void AddQuantBlock(int qtype, garri qvars) {
    assert(qtype=='A' || qtype=='E' || qtype=='F');
    int QuantGate = pGloProb->OutGlit;
    int ixQb = pGloProb->QuantBlk.n;
    assert(qtype != pGloProb->QuantBlk.last().qtype);
    pGloProb->QuantBlk.AppendBlank();
    pGloProb->QuantBlk[ixQb].qtype = qtype;
    pGloProb->QuantBlk[ixQb].ix = ixQb;
    pGloProb->QuantBlk[ixQb].bat = QuantGate;
    VarActOrder.add_qb(ixQb);
    foreach (it, qvars) {
        int qvar = *it;
        add_var_to_qblock(qvar, ixQb);
    }
    QuantPfx.blks.append(pGloProb->QuantBlk[ixQb]);
    if (ixQb > MAX_QB) {
        die("Too many quantification blocks.");
    }
}

void add_gate_to_qb(int ixExpr, garri args) {
    for (int qtype=0; qtype <= 1; qtype++) {
        int MaxQb = 0;
        foreach (itArg, args) {
            int CurQb = QbFromLit[ghosted_or_pass(*itArg, qtype)];
            assert(CurQb != 0);
            if (MaxQb < CurQb) {
                MaxQb = CurQb;
            }
        }
        if (MaxQb > InnermostInputQb) {
            MaxQb = InnermostInputQb;
        }
        int CurQb = MaxQb;
        while (pGloProb->QuantBlk[CurQb].qtype != QtypeLetrFromNum(qtype)) {
            CurQb++;
            assert(CurQb < pGloProb->QuantBlk.n);
        }
        assert(pGloProb->QuantBlk[CurQb].qtype == QtypeLetrFromNum(qtype));
        add_var_to_qblock(ghosted(absv(ixExpr), qtype), CurQb);
    }
}

double CalcGateExpFactor() {
    if (!AllowCegar && !AllocCegarVars) {
        return 1;
    }
    double THRES1 = 10000;
    double THRES2 = 500000;
    double pct = 0;
    if (false) {
    } else if (pGloProb->LastOrigGlit < THRES1) {
        pct = 1;
    } else if (pGloProb->LastOrigGlit < THRES2) {
        pct = ((THRES2 - pGloProb->LastOrigGlit) / THRES2);
        pct = pct * sqrt(pct);
    }
    double factor = 1 + pct * 24;
    return factor;
}

/****************************************************************************/

Fmla* fmla_from_gq_parser(GhostQ_Parser::Parser& parser, int raw_gate) {
    if (raw_gate < 0) {
        return fmla_from_gq_parser(parser, -raw_gate)->negate();
    }
    if (!parser.GateDefs.count(raw_gate)) {
        Fmla* ret = int_to_fmla_lit(raw_gate);
        raw_gate_to_fmla[raw_gate] = ret;
        return ret;
    }
    if (raw_gate_to_fmla[raw_gate] != NULL) {
        return raw_gate_to_fmla[raw_gate];
    }
    const char* op = parser.GateDefs[raw_gate].first;
    vector<int>& arg_ints = parser.GateDefs[raw_gate].second; 
    vector<Fmla*> arg_fmlas = vector<Fmla*>();
    foreach (it_arg, arg_ints) {
        arg_fmlas.push_back(fmla_from_gq_parser(parser, *it_arg));
    }
    Fmla* ret = RawFmla(op, arg_fmlas);

    
    
    auto(itQuantPair, parser.GateQuants.find(raw_gate));
    if (itQuantPair != parser.GateQuants.end()) {
        vector<GhostQ_Parser::PrefixBlock> blks = itQuantPair->second;
        for (int i = blks.size() - 1; i >= 0; i--) {
            char qtype_letr = blks[i].first;
            FmlaOp::op qtype_op;
            if      (qtype_letr == 'e') {qtype_op = FmlaOp::EXISTS;}
            else if (qtype_letr == 'a') {qtype_op = FmlaOp::FORALL;}
            else if (qtype_letr == 'f') {qtype_op = FmlaOp::FREE;}
            else {die_f("Bad qtype for gate %i.", raw_gate); abort();}
            vector<Fmla*> qvars = vector<Fmla*>();
            foreach (itVar, blks[i].second) {
                qvars.push_back(fmla_from_gq_parser(parser, *itVar));
            }
            ret = RawFmla(qtype_op, RawFmla(FmlaOp::LIST, qvars), ret);
        }
    }


    raw_gate_to_fmla[raw_gate] = ret;
    return ret;
}
Fmla* fmla_from_gq_parser(GhostQ_Parser::Parser& parser) {
    return fmla_from_gq_parser(parser, parser.OutputGateLit);
}



/*****************************************************************************
* ParseFile.    }}}{{{1
*****************************************************************************/
ProbInst ParseFile(char* filename) {
    GhostQ_Parser::Parser parser;
    parser.parse(filename);
    pNumClauses = &pGloProb->clauses.n;
    ProbInst& prob = *pGloProb;

    prob.LastInpLit = parser.LastInputVar | 1;
    prob.LastOrigGlit = parser.LastGateVar | 1;
    prob.OutGlit = parser.OutputGateLit;
    prob.PreprocTimeMilli = parser.PreprocTimeMilli;
    VarNames = parser.VarNames;

    int RawLastInpLit = prob.LastInpLit;
    prob.RawLastInpLit = RawLastInpLit;
    prob.OutGlit = frob_raw_gate(prob.OutGlit);
    prob.LastOrigGlit = frob_raw_gate(absv(prob.LastOrigGlit)) + 3;
    prob.LastGlit = (int)(prob.LastOrigGlit * CalcGateExpFactor() + 10000) | 1;
    prob.LastInpLit = prob.LastGlit;
    prob.FirstGateVar = prob.LastGlit;
    prob.NextNewVar = prob.LastOrigGlit + 1;
    prob.NoMoreVars = false;


    prob.LastPlit = prob.LastGlit;

    prob.clauses.init(0, prob.LastGlit*2 + 4000);
    prob.clauses.AppendBlank();
    prob.LitHave = talloc(garr<GsT*>, prob.LastPlit+1);
    prob.clause_id_to_strat.init(0);
    GateDefnB.init(prob.LastPlit+1);
    ReEvalArr = talloc(short, prob.LastPlit+1);
    VarActivity = garr<double>(prob.LastPlit+1);
    GlitToGs = talloc(GsT*, prob.LastPlit+1);
    GhostToGs = garr<GsT*>(prob.LastPlit+1);
    raw_gate_to_fmla = garr<Fmla*>(prob.LastPlit+1);
    QbFromLit = talloc(short, prob.LastPlit+1);
    prob.QuantBlk.init(0);
    prob.QuantBlk.AppendBlank();
    assert(prob.QuantBlk[0].ix == 0);

    for (int lit = 0; lit <= prob.LastOrigGlit; lit++) {
        GhostToGs[lit] = (lit <= prob.RawLastInpLit ? INPUT_LIT : NON_INPUT);
    }

    pGloAsgn->init();
    pGloAsgn->LitWatch = new GwSetT[prob.LastPlit+1];
    #if USE_WATCH_ARR
    for (int ix = 0; ix <= prob.LastPlit; ix++) {
        pGloAsgn->LitWatch[ix].wlit = ix;
    }
    #endif

    foreach (itGatePair, parser.GateDefs) {
        int GateNum = itGatePair->first;
        GhostQ_Parser::GateDef GateDef = itGatePair->second;
        const char* sOp = GateDef.first;
        if (!(StrEqc(sOp, "and") || StrEqc(sOp, "or"))) {
            die_f("Error in def'n of gate %i: Unexpected '%s'.\n", GateNum, sOp);
        }
    }

    OrigFmla = fmla_from_gq_parser(parser);
    
    if (JustPrint) {
        return prob;
    }
    
    /* Process quantifier blocks */
    int PrevQvar = 0;
    int PrevMaxLit = 0;
    int CurMaxLit = 0;
    foreach (itQuantPair, parser.GateQuants) {
        int QuantGate = itQuantPair->first;
        vector<GhostQ_Parser::PrefixBlock> blks = itQuantPair->second;
        QuantGate = frob_raw_gate(QuantGate);
        if (QuantGate != prob.OutGlit) {
            die("Non-prenex forms are no longer supported.");
        }
        foreach (itBlk, blks) {
            garri qvars;
            qvars.init();
            foreach (itVar, itBlk->second) {
                int qvar = *itVar;
                if (qvar <= 0) {
                    die_f("In quantifier prefix: Variable number %i is not positive.\n", qvar);}
                if (qvar % 2 != 0) {
                    die_f("In quantifier prefix: Variable number %i is not even.\n", qvar);}
                if (qvar != PrevQvar+2) {
                    die_f("In quantifier prefix: CurQvar = %i != %i = PrevQvar+2\n", qvar, PrevQvar+2);}
                PrevQvar += 2;
                if (!prob.IsValidInpVar(qvar)) {
                    die_f("In quantifier prefix: Variable number %i is out of range.\n", qvar);}
                if (qvar < PrevMaxLit) {
                    die_f("In quantifier prefix: Var %i (inner QB) must be numbered higher than var %i (outer QB).\n", qvar, PrevMaxLit);
                }
                qvars.append(qvar);
                CurMaxLit = max(CurMaxLit, qvar);
            }
            PrevMaxLit = CurMaxLit;
            char qtype = toupper(itBlk->first);
            if (qtype=='F') {
                if (!AllowFree) {
                    die_f("Must specify '-allow-free' option to allow free variables.\n");
                }
                if (pGloProb->QuantBlk.n > 1) {
                    die_f("Free variables must be in outermost block.\n");
                }
            }
            AddQuantBlock(qtype, qvars);
            qvars.dealloc();
        }
    }

    /* Proof Log */ 
    if (PrfLog) {
        if (print_prf_log_free) {
            fprintf(PrfLog, "free(");
            if (pGloProb->QuantBlk[1].qtype == 'F') {
                fprintf(PrfLog, "[");
                int col = 6;
                foreach (it, pGloProb->QuantBlk[1].lits) {
                    col += PrintLitRaw(*it, PrfLog);
                    col += fprintf(PrfLog, ", ");
                    if (col > 76) {
                        fprintf(PrfLog, "\n  ");
                        col = 3;
                    }
                }
                fprintf(PrfLog, "]");
            } else {
                fprintf(PrfLog, "[]");
            }
            fprintf(PrfLog, ",\n");
        }
        fprintf(PrfLog, "list(\n");
    }


    /* Process gate definitions. */
    foreach (itGatePair, parser.GateDefs) {
        int GateNum = itGatePair->first;
        if (GateNum <= 0) {
            die_f("In defn of gate %i: Defined gate number %i is not positive.\n", GateNum);}
        if (GateNum % 2 != 0) {
            die_f("In defn of gate %i: Defined gate number %i is not even.\n", GateNum);}
        GhostQ_Parser::GateDef GateDef = itGatePair->second;
        const char* sOp = GateDef.first;
        vector<int> args = GateDef.second;
        {
            set<int> TmpSet(args.begin(), args.end());
            for (uint i=0; i < args.size(); i++) {
                if (TmpSet.count(-args[i]) != 0) {
                    die_f("Defn of gate %i contains contradictory literals: %i and its negation.\n", GateNum, args[i]);
                }
            }
            if (TmpSet.size() < (uint) args.size()) {
                die_f("Defn of gate %i contains a duplicated argument.\n", GateNum);
            }
        }
        if (args.size() == 1) {
            die_f("Defn of gate %i needs at least two distinct arguments.\n", GateNum);
        }
        for (uint i=0; i < args.size(); i++) {
            if (abs(args[i]) >= GateNum) {
                die_f("In defn of gate %i: Argument %i is not numbered less than parent gate number.\n", GateNum, args[i]);
            }
            if (abs(args[i]) > parser.LastInputVar && parser.GateDefs.count(abs(args[i])) == 0) {
                die_f("In defn of gate %i: Argument %i is undefined.\n", GateNum, args[i]);
            }
        }

        int ixExpr = frob_raw_gate(GateNum);
        assert(prob.IsValidExprIx(ixExpr));
        foreach (it, args) {
            *it = encv(*it);
            if (*it > RawLastInpLit) {
                *it = frob_raw_gate(*it);
            }
        }
        if (StrEqc(sOp, "AND")) {
        } else if (StrEqc(sOp, "OR")) {
            // [x = a || b || c] is equivalent to [~x = ~a && ~b && ~c].
            ixExpr = negv(ixExpr);
            sOp = "AND";
            foreach (it, args) {*it = negv(*it);}
        } else {
            die_f("Error in def'n of gate %i: Unexpected '%s'.\n", GateNum, sOp);
        }
        prob.FirstGateVar = min(prob.FirstGateVar, absv(ixExpr));
        garri gargs = garri();
        foreach (it_arg, args) {
            gargs.append(*it_arg);
        }
        GateDefn.setval(ixExpr, gargs);
        GateDefnB[ixExpr].op = FmlaOp::AND;
        GateDefnB[ixExpr].args = gargs;
    }

    InnermostInputQb = prob.QuantBlk.n - 1;

    for (int q = 0; q <= 1; q++) {
        garri qvars = garri();
        AddQuantBlock(prob.QuantBlk.last().qtype=='E' ? 'A' : 'E', qvars);
        int CurQtype = QtypeNumFromLetr(prob.QuantBlk.last().qtype);
        ExtraQb[CurQtype] = prob.QuantBlk.last().ix;
    }

    for (int ixExpr = 0; ixExpr <  GateDefnB.n; ixExpr++) {
        garri args = GateDefnB[ixExpr].args;
        if (args.arr == NULL) continue;
        for (int qtype=0; qtype <= 1; qtype++) {
            int qvar = ghosted(absv(ixExpr), qtype);
            assert(QbFromLit[qvar] == 0);
        }
    }
    for (int ixExpr = 0; ixExpr <  GateDefnB.n; ixExpr++) {
        if (NoUnivGhost) break;
        garri args = GateDefnB[ixExpr].args;
        if (args.arr == NULL) continue;
        add_gate_to_qb(ixExpr, args);
    }

    for (int ixExpr = GateDefn.n - 1; ixExpr >= 0; ixExpr -= 1) {
        garri args = GateDefn[ixExpr];
        if (args.arr == NULL) continue;
        if (!NoUnivGhost) {
            pGloAsgn->NewDefnGsAlt(ixExpr, args);
        } else {
            if (ixExpr == prob.OutGlit) continue;
            assert(GateDefn[prob.OutGlit].bin_search(negv(ixExpr)));
            garri NegArgs = garri(args.n);
            for (int i=0; i < args.n; i++) {
                NegArgs[i] = negv(args[i]);
            }
            GsT* pCurLs = pGloAsgn->NewLrnGs_Mixed(args, 0);
            pCurLs->IsLearned = false;
            NegArgs.dealloc();
        }
    }
     
    int NumDefn = GateDefn.n;
    for (int ixExpr = 0; ixExpr < NumDefn; ixExpr++) {
        if ((ixExpr & 2) != 0) continue;
        garri args = GateDefn[ixExpr];
        if (args.arr == NULL) continue;
        assert(ghosted(ixExpr, 0) == ixExpr);
        garri new_args(args.n);
        for (int i=0; i < args.n; i++) {
            new_args[i] = ghosted_or_pass(args[i], 1);
        }
        GateDefn.setval(ghosted(ixExpr, 1), new_args);
    }

    max_learnts = ((double) pGloProb->clauses.n) * learntsize_factor;

    LastOrigClauseID = pGloProb->clauses.n;

    return prob;
}


void AsgnT::PrintStats() {
    double x;
    double CpuTime = (GetUserTimeMicro() - StartTime) / 1000000.0;
    x=NumBigBt;          printf("Conflicts:    %10.0f  (%9.0f / sec)\n", x, x / CpuTime);
    x=NumDecs;           printf("Decisions:    %10.0f  (%9.0f / sec)\n", x, x / CpuTime);
    x=NumProps;          printf("Propagations: %10.0f  (%9.0f / sec)\n", x, x / CpuTime);
    x=NumWatchFixes;     printf("Watch fixes:  %10.0f  (%9.0f / sec)\n", x, x / CpuTime);
    x=NumWatchCleanups;  printf("Watch cleans: %10.0f  (%9.0f / sec)\n", x, x / CpuTime);
    x=max_learnts;       printf("max_learnts:  %10.0f  \n", x);
    x=NumReducs;         printf("NumReducs:    %10.0f  \n", x);
}


/*****************************************************************************
* main.    }}}{{{1
*****************************************************************************/

int main(int argc, char** argv) {
    
    GloRandSeed = 1;
    int ShowTime = 1;

    init_fmla();

    if (argc < 2 || (StrEqc(argv[1], "-h") || StrEqc(argv[1], "--help"))) {
        fprintf(stderr, "Usage: %s infile [OPTIONS]\n", argv[0]);
        fprintf(stderr, 
            "Options:\n"
            " '-allow-free' Allow free variables; disables certain optimizations.\n"
            " '-strat FILE' Generate a strategy for the winning player.\n"
            " '-cegar 0'    Don't use CEGAR learning.\n"
            " '-cegar 1'    Use CEGAR learning.\n"
            " '-pure-bdd'   Print the raw BDD, without simplifications.\n"
            " '-write-qcir' Write the answer in QISCAS format.  (UNTESTED)\n"
            " '-s N'        Use N as the seed for the random number generator.\n"
            " '-stime'      Seed the random number generator using the current time.\n"
            " '-s-cnf'      Print 's cnf 0' if instance is false, 's cnf 1' if true.\n"
            " '-q1', '-q2'  Quiet modes.\n"
            " '-qtime'      Show time even in quiet mode.\n"
            " '-no-time'    Don't show elapsed time. (For regression testing.)\n"
            " '-time-out N' Abort after N seconds.\n"
            " '-pt'         Include preprocessing time in time limit.\n"
            " '-plog FILE'  Write a log to FILE.\n"
            " '-just-print' Write the original formula and quits.\n"
            " '-d N'        Print debugging info; higher N for more info.\n"
            " '-san-chk N'  Higher N means more sanity checks.  Default is __.\n"
            " See source code for additional options (mostly for debugging).\n"
        );
        exit(1);
    }


    char* in_filename = argv[1];
    if (!StrEqc(in_filename, "-") && access(in_filename, F_OK) == -1) {
        fprintf(stderr, "File '%s' does not exist.\n", in_filename);
        exit(1);
    }

    /* Parse command-line options. */
    bool AddPreprocTime = false;
    bool DumpFinalAsgn = false;
    bool CondensedOut = false;
    bool s_cnf = false;
    bool PureBDD = false;
    bool WriteQiscas = false;
    char* PlogName = NULL;
    char* StratFilename = NULL;
    int ixArg = 2;
    for (; ixArg < argc; ixArg++) {
        char* sArg = argv[ixArg];
        if (sArg[0] == '-' && sArg[1] == '-') {
            sArg += 1;
        }
        if (false) {}
        else if (StrEqc(sArg, "-pt")) {AddPreprocTime = true;}
        else if (StrEqc(sArg, "-s-cnf")) {s_cnf = true;}
        else if (StrEqc(sArg, "-Dump-Final-Asgn")) {DumpFinalAsgn = true;}
        else if (StrEqc(sArg, "-q2")) {QuietMode = true; ShowTime = 0;}
        else if (StrEqc(sArg, "-q1")) {CondensedOut = true;}
        else if (StrEqc(sArg, "-qtime")) {ShowTime = true;}
        else if (StrEqc(sArg, "-No-Time")) { ShowTime = 0; }
        else if (StrEqc(sArg, "-No-Restart")) {NoRestart = true;}
        else if (StrEqc(sArg, "-No-Univ-Ghost")) { NoUnivGhost = 1; }
        else if (StrEqc(sArg, "-stime")) { GloRandSeed = RandSeedFromTime(); }
        else if (StrEqc(sArg, "-Var-Ord-Fix")) { VarOrdFix = 1; }
        else if (StrEqc(sArg, "-Allow-Free")) { AllowFree = 1; }
        else if (StrEqc(sArg, "-f")) { AllowFree = 1; }
        else if (StrEqc(sArg, "-Pure-BDD")) { PureBDD = 1; }
        else if (StrEqc(sArg, "-write-qcir")) { WriteQiscas = 1; }
        else if (StrEqc(sArg, "-Alloc-Cegar-Vars")) { AllocCegarVars = 1; }
        else if (StrEqc(sArg, "-Just-Print")) { JustPrint = true; }
        else if (StrEqc(sArg, "-raw-strat")) { use_raw_strat = 1; }
        else if (ixArg+1 >= argc) {
            die_f("The option '%s' doesn't exist, or it requires a parameter.\n", sArg);
        }
        else if (StrEqc(sArg, "-cegar")) { AllowCegar = atoi(argv[++ixArg]); }
        else if (StrEqc(sArg, "-monotone")) { UseMonotone = atoi(argv[++ixArg]); }
        else if (StrEqc(sArg, "-plog")) { PlogName = argv[++ixArg]; }
        else if (StrEqc(sArg, "-strat")) { StratFilename = argv[++ixArg]; }
        else if (StrEqc(sArg, "-Max-Learn-Factor")) { learntsize_factor = atof(argv[++ixArg]); }
        else if (StrEqc(sArg, "-s") || StrEqc(sArg, "-seed")) {
            assert(++ixArg < argc);
            GloRandSeed = atoi(argv[ixArg]);
        }
        else if (StrEqc(sArg, "-d") || StrEqc(sArg, "-Dbg-Lev")) {
            assert(++ixArg < argc);
            #ifndef GloDbgLev
            GloDbgLev = atoi(argv[ixArg]);
            #else
            die("Must recompile with GloDbgLev not #define'd to use this option.\n");
            #endif
        }
        else if (StrEqc(sArg, "-San-Chk")) {
            assert(++ixArg < argc);
            #ifndef NO_SAN_CHK
            GloSanChk = atoi(argv[ixArg]);
            #else
            die("Must recompile without NO_SAN_CHK #define'd to use this option.\n");
            #endif
        }
        else if (StrEqc(sArg, "-Restart-Cycle")) {
            assert(++ixArg < argc);
            RestartCycle = atoi(argv[ixArg]);
        }
        else if (StrEqc(sArg, "-Time-Out")) {
            assert(++ixArg < argc);
            GloTimeOut = (long long)(atof(argv[ixArg])*1000000);
        }
        else {
            printf("Unrecognized option: '%s'\n", sArg);
            exit(1);
        }
    }

    if (StratFilename) {
        if (!AllowFree) {die("Option '-strat' requires option '-allow-free'.\n");}
        if (AllowCegar) {die("Option '-strat' is incompatible with option '-cegar'.\n");}
        StratFile = fopen(StratFilename, "w");
    }

    if (PlogName) {
        if (!AllowFree) {die("Option '-plog' requires option '-allow-free'.\n");}
        if (AllowCegar) {die("Option '-plog' is incompatible with option '-cegar'.\n");}
        PrfLog = fopen(PlogName, "w");
    }


    if (GloDbgLev > 100) printf(
        "\n\n##############################################################################\n\n");

    GsT* pFinalSeq = NULL;
    FmlaT FreeFmla = NOT_DONE;
    {
        /* Seed the random number generator. */
        srand(GloRandSeed);
        
        /* Read the input QBF file. */
        try {
            ParseFile(in_filename);
            if (JustPrint) {
                if (WriteQiscas) {
                    OrigFmla->write_qcir(stdout);
                } else {
                    OrigFmla->write(stdout);
                    printf("\n");
                }
                exit(0);
            }
        } catch (char const* pErrMsg) {
            printf("%s\n", pErrMsg);
            fprintf(stderr, "%s\n", pErrMsg);
            exit(1);
        }

        StartTime = GetUserTimeMicro();
        if (AddPreprocTime) {
            StartTime -= (((long long) pGloProb->PreprocTimeMilli) * 1000);
        }
        garr<GsT*>& clauses = pGloProb->clauses;
        for (int i=1; i < clauses.n; i++) {
            pGloAsgn->SimpFixWatch(clauses[i]);
        }

        QbFromLit[ZERO_LIT] = ZERO_DECLEV;
        DupLrnTest.clear();

        if (GloDbgLev >= 2020) {printf("OutGlit: %i\n", pGloProb->OutGlit);}

        if (!QuietMode) {
            printf("#Seed: %2i. ", GloRandSeed);
            fflush(stdout);
        }
        if (GloDbgLev >= 1000) {printf("\n");}

        /* Call the solver procedure. */
        pGloAsgn->FreeFmla = NOT_DONE;
        pGloAsgn->PreSolve();
        pFinalSeq = pGloAsgn->solve();
        FreeFmla = pFinalSeq->FreeFmla;
        // if (pGloAsgn->HasConflict()) {
        //     FreeFmla = pGloAsgn->FreeFmla;
        // } else {
        //     pGloAsgn->FreeFmla = NOT_DONE;
        //     FreeFmla = pGloAsgn->solve();
        // }
        FixNeedNewl();
        double CpuTime = (GetUserTimeMicro() - StartTime) / 1000000.0;
        const char* TruthStr = NULL;
        int TruthInt = 999;
        if (FreeFmla == FmlaTrue) {
            TruthStr = ("true. ");
            TruthInt = 1;
        } else if (FreeFmla == FmlaFalse) {
            TruthStr = ("false.");
            TruthInt = 0;
        } else {
            TruthStr = ("free. ");
        }
        if (QuietMode) {
            if (!s_cnf) {
                printf("%s", TruthStr);
            }
            if (ShowTime) {
                printf(" %0.3f", CpuTime);
            }
            printf("\n");
        } else if (CondensedOut) {
            printf("%s Bt:%5i, Dec:%5lli.  T: %0.3f s.\n", 
                TruthStr, NumBigBt, NumDecs, ShowTime * CpuTime);
        } else {
            printf("%s Bt:%5i, D:%5lli.  R:%4i, P:%7lli, w:%9lli, C:%3i, T: %0.3f s.\n", 
                TruthStr, NumBigBt, NumDecs, pGloAsgn->NumRestarts,
                NumProps,
                NumWatchFixes,
                NumCegLrn,
                ShowTime * CpuTime);
            if (AllowFree) {
                if (!PureBDD) {
                    FreeFmla = FreeFmla->simp_ite();
                }
                if (WriteQiscas) {
                    FreeFmla->write_qcir(stdout);
                } else {
                    FreeFmla->write(stdout);
                    printf("\n");
                }
                //assert(FreeFmla->to_bdd() == OrigFmla->to_bdd());
            }
        }
        if (s_cnf) {
            printf("s cnf %i\n", TruthInt);
        }

        if (DumpFinalAsgn) {
            pGloAsgn->DumpAsgn();
        }
        if (GloDbgLev >= 100) {
            pGloAsgn->PrintStats();
            if (GloDbgLev >= 2000) {
                pGloAsgn->dump(0);
            }
        }
        if (0 && GloDbgLev >= 100) {
            garri& TopLits = pGloAsgn->UndoListByDL[ZERO_DECLEV];
            printf("Top unit lits: [");
            for (int i=0; i < TopLits.n; i++) {
                int CurLit = TopLits[i];
                if (!IsInputLit(CurLit)) continue;
                if (i != 0) printf(", ");
                printf("%i(%c)", decv(CurLit), tolower(QtypeFromLit(CurLit)));
            }
            printf("]\n");
        }
    }
    if (PrfLog) {
        if (print_prf_log_free) {fprintf(PrfLog, ")");}
        fprintf(PrfLog, ")\n");
    }
    if (StratFile) {
        pFinalSeq->get_strat_as_list_fmla()->write(StratFile);
        fprintf(StratFile, "\n");
    }
    if (FreeFmla == FmlaTrue) {
        return 10;
    } else if (FreeFmla == FmlaFalse) {
        return 20;
    } else {
        return 99;
    }
    return 0;

}
