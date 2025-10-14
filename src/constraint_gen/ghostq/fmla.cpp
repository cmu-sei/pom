/******************************************************************************
* Author: Will Klieber (Carnegie Mellon University)
******************************************************************************/

/*
 * TODO: 
 *  - Print warning/error if junk at end of file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

//#include "minisat/core/Solver.h"

#include <vector>
#include <map>
#include <set>
#include <algorithm>

//#include <ext/hash_map>
//using namespace __gnu_cxx;

#define FMLA_CPP
#include "fmla.hh"

using namespace FmlaParserNS;
using namespace std;

#define talloc(type, n) ((type*) calloc(n, sizeof(type)))
#define auto(var, init) typeof(init) var = (init)

#define foreach(var, container) \
  for (auto(var, (container).begin());  var != (container).end();  ++var)

#define LEX_ERR(str) {fprintf(stderr, "%s\n", str); abort();}

static void rawstop() {}    // Place a breakpoint here with gdb.
static void (*volatile stop)() = &rawstop;  // For debugging; optimizer won't remove.

static int die_f(const char* fmt, ...)  __attribute__ ((noreturn));
static int die_f(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}
static int printf_err(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return ret;
}

static int die(const char* msg="")  __attribute__ ((noreturn));
static int die(const char* msg) {
    fprintf(stderr, "%s", msg);
    abort();
}

static inline int StrEqc(const char* s1, const char* s2) {
    // Precond: At least one of the strings is not null.
    if ((s1 == NULL) ^ (s2 == NULL)) {return 0;}
    return (strcasecmp(s1, s2) == 0);
}

static char* StrFmt(const char* fmt, ...) { 
    va_list ap;
    int buf_size = 16*1024;
    char* pOut = talloc(char, buf_size);
    va_start(ap, fmt);
    vsnprintf(pOut, buf_size - 1, fmt, ap);
    va_end(ap);
    return pOut;
}

static long long GetUserTimeMicro() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    struct timeval tv = usage.ru_utime;
    return ((long long) tv.tv_sec) * 1000000 + (tv.tv_usec);
}

static long long GetClockTimeMicro() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((long long) tv.tv_sec) * 1000000 + (tv.tv_usec);
}


/***********************************************************************/

struct lt_str {
    bool operator()(char const *a, char const *b) const {
        return (strcmp(a, b) < 0);
    }
};

struct lt_fmla_ptr {
    bool operator()(Fmla *x, Fmla *y) {
        return *x < *y;
    }
};

bool fn_lt_fmla_ptr(Fmla *x, Fmla *y) {
    return *x < *y;
}

struct eq_fmla {
    bool operator()(Fmla *x, Fmla *y) {
        return *x == *y;
    }
};

struct hash_fmla {
    bool operator()(Fmla *x) {
        return x->hash;
    }
};


typedef const char* sym;

class SymbolTable {
    public:
    sym operator[](const char*);
    map<const char*, const char*, lt_str> table;
    //hash_map<const char*, const char*> table;
};

sym SymbolTable::operator[](const char* str) {
    auto(it, this->table.find(str));
    if (it != this->table.end()) {
        return it->second;
    } else {
        sym ret = strdup(str);
        this->table[ret] = ret;
        return ret;
    }
}

SymbolTable SymTab;





/*****************************************************************************
* Lexer Stuff
*****************************************************************************/

FmlaLexer::FmlaLexer() {
    tmp_file = NULL;
    infile = NULL;
    filename = "";
    CurLine = (sap){NULL, 0};
    CurTok  = (sap){NULL, 0};
    SavedTok = (sap){NULL, 0};
    CurPos = 0;
    LineNum = 0;
    SaveComments = false;
}

FmlaLexer::~FmlaLexer() {
    if (infile) {
        //fprintf(stderr, "(closing %p)", infile);
        fclose(infile); 
        infile = NULL;
    }
}

FmlaLexer* FmlaLexer::new_from_file(const char* _filename) /*NO_PROTO*/ {
    FmlaLexer* ret = new FmlaLexer();
    ret->init_from_file(_filename);
    return ret;
}

void FmlaLexer::init_from_str(const char* str) {
    filename = "(string)";
    tmp_file = tmpfile();
    fprintf(tmp_file, "%s\n", str);
    fflush(tmp_file);
    fseek(tmp_file, 0, SEEK_SET);
    infile = tmp_file;
    AdvLine();
    AdvTok();
};

void FmlaLexer::init_from_file(const char* _filename) {
    filename = _filename;
    const char* ExprPfx = "-e ";
    if (StrEqc(filename, "-") || StrEqc(filename, "stdin")) {
        infile = stdin;
    } else if (strncmp(filename, ExprPfx, strlen(ExprPfx)) == 0) {
        init_from_str(filename + strlen(ExprPfx));
        return;
    } else {
        infile = fopen(filename, "r");
        //fprintf(stderr, "(opening %p)", infile);
    }
    if (infile == 0) {
        LEX_ERR(StrFmt("File '%s' does not exist.", filename));
    }
    AdvLine();
    AdvTok();
};

void FmlaLexer::AdvLine() {
    CurLine.n = getline(&CurLine.p, (size_t*)&CurLine.n, infile);
    if (CurLine.n > 0 && CurLine.p[CurLine.n - 1] != '\n') {
        fprintf(stderr, "Line %i is not terminated with a newline (LF) character.\n",
            LineNum);
        exit(1);
    }
    CurPos = 0;
    LineNum++;
}

bool FmlaLexer::IsEOF() {
    return (CurLine.n == -1);
}

char FmlaLexer::peek() {
    if (CurPos >= CurLine.n) {
        return '\0';
    } else {
        return CurLine.p[CurPos];
    }
}

char* FmlaLexer::StrCurTokPos() {
    /* For printing error messages. */
    return StrFmt("line %i, col %i-%i", LineNum, CurPos - CurTok.n + 1, CurPos);
}

void FmlaLexer::AdvTok() {
    int InitPos;
    again:
    if (CurLine.n == -1) {
        CurTok = (sap){NULL, -1};
        return;
    }
    if (CurPos >= CurLine.n) {
        AdvLine();
        goto again;
    }
    InitPos = CurPos;
    switch (CurLine.p[CurPos++]) 
    {
        case ' ': case '\t': case ',':
            goto again;

        case '\n': 
            goto again;
        
        case '\r':  // For MS-DOS text files
            goto again;

        case '#':
            // Lines beginning with "#" are comment lines
            CurPos = CurLine.n - 1;
            if (SaveComments) {
                CurTok.n = CurLine.n - InitPos - 1;
                goto copytok;
            } else {
                goto again;
            }


        case '(': case ')':
        case '[': case ']':
        case ':': case '-':
        case ';': case '=':
            CurTok.n = 1;
            goto copytok;

        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '0' ... '9':
        case '_':  case '$':  case '/':  case '\\':
            while (true) {
              assert(CurPos <= CurLine.n);
              unsigned char CurChar = (unsigned char) CurLine.p[CurPos++];
              switch (CurChar) {
                case '0' ... '9':  case 'a' ... 'z':  case 'A' ... 'Z':  
                case '_':  case '$':
                case '-':  case '.':  case '@':
                case '/':  case '\\':
                case '<':  case '>':
                case '*':
                //case '[':  case ']':
                case 128 ... 255:
                  continue;
                default:
                  CurPos--;
                  CurTok.n = CurPos - InitPos;
                  goto copytok;
              }
            }


        default: {
            char* pChrStr;
            unsigned char chr = CurLine.p[CurPos-1];
            if ((32 <= chr) && (chr <= 126)) {
                pChrStr = StrFmt("'%c'", chr);
            } else {
                pChrStr = StrFmt("0x%X", chr);
            }
            fprintf(stderr, "Invalid character (%s) on line %i, column %i.\n",
                pChrStr, LineNum, CurPos);
            exit(1);
        }
    }
    copytok:
    CurTok.p = (char*) realloc(CurTok.p, CurTok.n+1);
    memcpy(CurTok.p, &CurLine.p[InitPos], CurTok.n);
    CurTok.p[CurTok.n] = 0;
    assert((int)strlen(CurTok.p) == CurTok.n);
}

void FmlaLexer::SaveTok() {
    SavedTok.p = (char*) realloc(SavedTok.p, CurTok.n+1);
    memcpy(SavedTok.p, CurTok.p, CurTok.n);
    SavedTok.p[SavedTok.n] = 0;
    assert((int)strlen(CurTok.p) == CurTok.n);
}

void FmlaLexer::EatTok(const char* expected) {
    if (!StrEqc(CurTok.p, expected)) {
        LEX_ERR(StrFmt("Parsing error (%s): Expected '%s', but found '%s'.",
            StrCurTokPos(), expected, CurTok.p));
    }
    AdvTok();
}


/****************************************************************************/

extern "C" int fmla_op_str_to_enum(char* op_str) {
    return FmlaOp::str_to_enum(op_str);
}

extern "C" char* fmla_op_enum_to_str(int op_num) {
    return (char*) FmlaOp::enum_to_str((FmlaOp::op) op_num);
}

Fmla Fmla::operator=(Fmla& src) {
    assert(false);
}

bool Fmla::operator==(Fmla& other) const {
    if (this->hash != other.hash || this->op != other.op || this->num_args != other.num_args) {
        return false;
    }
    for (int i=0; i < this->num_args; i++) {
        if (this->arg[i] != other.arg[i]) {
            return false;
        }
    }
    return true;
}

bool Fmla::operator<(Fmla& other) const {
    if (this->op < other.op) {return true;}
    if (this->op > other.op) {return false;}
    if (this->op == FmlaOp::VAR) {
        return (strcmp((char*) this->arg[0], (char*) other.arg[0]) < 0);
    }
    if (this->num_args < other.num_args) {return true;}
    if (this->num_args > other.num_args) {return false;}
    if (this->hash < other.hash) {return true;}
    if (this->hash > other.hash) {return false;}
    for (int i=0; i < this->num_args; i++) {
        if (this->arg[i] < other.arg[i]) {return true;}
        if (this->arg[i] > other.arg[i]) {return false;}
    }
    return false;
}

int hash_str(const unsigned char* str) {
    unsigned int hash = 0;
    int c;
    while (true) {
        c = *str++;
        if (c == 0) {break;}
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void Fmla::calc_hash() {
    #define ADD_TO_HASH(x) {cur_hash = ((cur_hash << 5) + cur_hash) + x;}
    unsigned int cur_hash = 0;
    ADD_TO_HASH(this->op);
    ADD_TO_HASH(this->num_args);
    if (this->op == FmlaOp::VAR) {
        ADD_TO_HASH(hash_str((unsigned char*)this->arg[0]));
    } else {
        for (int i=0; i < this->num_args; i++) {
            ADD_TO_HASH(this->arg[i]->id);
        }
    }
    this->hash = cur_hash;
}

typedef vector<Fmla*> FmlaPtrVector;
template <int N>
struct FmlaVecFixd {
    Fmla* arr[N];
    inline Fmla*& operator[](int i) {
        return arr[i];
    }
    inline int size() {
        return N;
    }
};

struct FmlaArr {
    int n;
    Fmla** arr;
    inline Fmla*& operator[](int i) {
        return arr[i];
    }
    inline int size() {
        return n;
    }
    /* Constructor */
    FmlaArr() : n(0), arr(NULL) {};
    FmlaArr(int _n, Fmla** _arr) : n(_n), arr(_arr) {};
};

//hash_map<Fmla*, Fmla*, hash_fmla, eq_fmla> fmla_cache_raw;
map<Fmla*, Fmla*, lt_fmla_ptr> fmla_cache_raw;
map<Fmla*, Fmla*, lt_fmla_ptr> op_cache;

Fmla* fmla_cache_raw_find(Fmla* phi) {
    auto(it, fmla_cache_raw.find(phi));
    if (it != fmla_cache_raw.end()) {
        return it->second;
    } else {
        return NULL;
    }
}
void fmla_cache_raw_insert(Fmla* phi) {
    fmla_cache_raw[phi] = phi;
}

Fmla* op_cache_find(Fmla* phi) {
    auto(it, op_cache.find(phi));
    if (it != op_cache.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

Fmla* fmla_true;
Fmla* fmla_false;
Fmla* fmla_error;

inline vector<Fmla*> FmlaPtrVec(Fmla* x0) {
    vector<Fmla*> ret(1, NULL);
    ret[0] = x0;
    return ret;
}
inline vector<Fmla*> FmlaPtrVec(Fmla* x0, Fmla* x1) {
    vector<Fmla*> ret(2, NULL);
    ret[0] = x0;
    ret[1] = x1;
    return ret;
}
inline vector<Fmla*> FmlaPtrVec(Fmla* x0, Fmla* x1, Fmla* x2) {
    vector<Fmla*> ret(3, NULL);
    ret[0] = x0;
    ret[1] = x1;
    ret[2] = x2;
    return ret;
}

// Fmla* fmla_arr = NULL;
// int last_fmla_num = 0;
// int max_fmla_num = 0;
// void init_fmla(int max_mem) {
//     max_fmla_mem = max_mem;
// }

int next_fmla_num = 1;

template <typename T>
Fmla* RawFmla(FmlaOp::op op, T& argvec) {
    Fmla* ret = (Fmla*) malloc(sizeof(Fmla) + sizeof(Fmla*) * argvec.size());
    ret->op = op;
    ret->num_args = argvec.size();
    switch (op) {
        case FmlaOp::NOT: 
        case FmlaOp::VAR: 
            assert(ret->num_args == 1); break;
        case FmlaOp::XOR: 
            assert(ret->num_args >= 1); break;
        case FmlaOp::IMPL:
            {
            assert(ret->num_args == 2);
            FmlaVecFixd<2> arr = FmlaVecFixd<2>();
            arr[0] = argvec[0]->negate();
            arr[1] = argvec[1];
            return RawFmla<FmlaVecFixd<2> >(FmlaOp::OR, arr);
            }
        case FmlaOp::EQ: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL: 
        case FmlaOp::FREE: 
            assert(ret->num_args == 2); break;
        case FmlaOp::ITE: 
            assert(ret->num_args == 3); break;
        case FmlaOp::GSEQ: 
            assert(ret->num_args == 3);
            assert(argvec[0]->op == FmlaOp::LIST);
            assert(argvec[1]->op == FmlaOp::LIST);
            break;
        case FmlaOp::RESOLVE: 
        case FmlaOp::INCLUDE: 
        case FmlaOp::NEWENV: 
            assert(ret->num_args >= 1);
            break;
        default: break;
    }
    for (int i=0; i < ret->num_args; i++) {
        ret->arg[i] = argvec[i];
    }
    ret->calc_hash();
    Fmla* cached = fmla_cache_raw_find(ret);
    if (cached != NULL) {
        free(ret);
        ret = cached;
    } else {
        ret->id = next_fmla_num++;
        fmla_cache_raw_insert(ret);
    }
    return ret;
}
Fmla* RawFmla(sym OpStr, vector<Fmla*> argvec) {
    return RawFmla<FmlaPtrVector>(FmlaOp::str_to_enum(OpStr), argvec);
}
Fmla* RawFmla(FmlaOp::op op, vector<Fmla*> argvec) {
    return RawFmla<FmlaPtrVector>(op, argvec);
}
Fmla* RawFmla(FmlaOp::op op, Fmla* arg0) {
    FmlaVecFixd<1> arr = FmlaVecFixd<1>();
    arr[0] = arg0;
    return RawFmla<FmlaVecFixd<1> >(op, arr);
}
Fmla* RawFmla(FmlaOp::op op, Fmla* arg0, Fmla* arg1) {
    FmlaVecFixd<2> arr = FmlaVecFixd<2>();
    arr[0] = arg0;
    arr[1] = arg1;
    return RawFmla<FmlaVecFixd<2> >(op, arr);
}
Fmla* RawFmla(FmlaOp::op op, Fmla* arg0, Fmla* arg1, Fmla* arg2) {
    FmlaVecFixd<3> arr = FmlaVecFixd<3>();
    arr[0] = arg0;
    arr[1] = arg1;
    arr[2] = arg2;
    return RawFmla<FmlaVecFixd<3> >(op, arr);
}

extern "C" Fmla* RawFmlaArr(FmlaOp::op op, int num_args, Fmla** arr_args) {
    vector<Fmla*> vec_args = vector<Fmla*>(&arr_args[0], &arr_args[num_args - 1]);
    return RawFmla(op, vec_args);
}

extern "C" Fmla* FmlaVar(const char* var) {
    FmlaVecFixd<1> arr = FmlaVecFixd<1>();
    arr[0] = (Fmla*) SymTab[var];
    return RawFmla<FmlaVecFixd<1> >(FmlaOp::VAR, arr);
}
Fmla* ConsFmla(FmlaOp::op op, Fmla* x1) {
    return ConsFmla(op, FmlaPtrVec(x1));
}
Fmla* ConsFmla(FmlaOp::op op, Fmla* x1, Fmla* x2) {
    return ConsFmla(op, FmlaPtrVec(x1, x2));
}
Fmla* ConsFmla(FmlaOp::op op, Fmla* x1, Fmla* x2, Fmla* x3) {
    return ConsFmla(op, FmlaPtrVec(x1, x2, x3));
}

Fmla* ConsFmla(FmlaOp::op op, vector<Fmla*> argvec) {
    switch (op) {
        case FmlaOp::ITE: {
            assert(argvec.size() == 3);
            Fmla* test = argvec[0];
            Fmla* tbra = argvec[1];
            Fmla* fbra = argvec[2];
            if (test->op == FmlaOp::NOT && test->arg[0]->op == FmlaOp::VAR) {
                test = test->arg[0];
                swap(tbra, fbra);
            }
            if (test->op == FmlaOp::TRUE)  {return tbra;} 
            if (test->op == FmlaOp::FALSE) {return fbra;} 
            if (tbra == fbra) {return tbra;} 
            return RawFmla(op, test, tbra, fbra);
        }
        case FmlaOp::AND:
        case FmlaOp::OR: {
            Fmla* base = (op == FmlaOp::AND ? fmla_true : fmla_false);
            Fmla* negbase = base->negate();
            vector<Fmla*> new_args;
            foreach (it_arg, argvec) {
                Fmla* arg = *it_arg;
                if (arg == base) {continue;}
                else if (arg == negbase) {return negbase;}
                else {new_args.push_back(arg);}
            }
            if (new_args.size() == 1) {
                return new_args[0];
            }
            if (new_args.size() == 0) {
                return base;
            }
            return RawFmla<FmlaPtrVector>(op, new_args);
        }
        case FmlaOp::EQ: {
            assert(argvec.size() == 2);
            if (argvec[0] == argvec[1]) {return fmla_true;}
            if (argvec[0] == fmla_true) {return argvec[1];}
            if (argvec[1] == fmla_true) {return argvec[0];}
            if (argvec[0] == fmla_false) {return argvec[1]->negate();}
            if (argvec[1] == fmla_false) {return argvec[0]->negate();}
        }
        case FmlaOp::XOR: {
            vector<Fmla*> new_args;
            int parity = false;
            foreach (it_arg, argvec) {
                Fmla* arg = *it_arg;
                if (arg == fmla_false) {continue;}
                else if (arg == fmla_true) {parity = !parity; continue;}
                else {
                    new_args.push_back(arg);
                }
            }
            if (new_args.size() == 0) {
                if (parity) {return fmla_true;}
                else {return fmla_false;}
            }
            if (parity) {
                new_args[0] = new_args[0]->negate();
            }
            return RawFmla<FmlaPtrVector>(op, new_args);
        }
        case FmlaOp::NOT: {
            assert(argvec.size() == 1);
            return argvec[0]->negate();
        }
        case FmlaOp::INCLUDE: {
            assert(argvec.size() == 1);
            return parse_fmla_file(argvec[0]->name_of_var());
        }
        case FmlaOp::NEWENV: {
            assert(argvec.size() == 1);
            return argvec[0];
        }
        default:
            break;
    }
    return RawFmla<FmlaPtrVector>(op, argvec);
}
Fmla* ConsFmla(sym OpStr, vector<Fmla*> argvec) {
    return ConsFmla(FmlaOp::str_to_enum(OpStr), argvec);
}

extern "C" Fmla* ConsFmlaArr(FmlaOp::op op, int num_args, Fmla** arr_args) {
    vector<Fmla*> vec_args = vector<Fmla*>(&arr_args[0], &arr_args[num_args - 1]);
    return ConsFmla(op, vec_args);
}

Fmla* Fmla::flatten_andor() {
    map<Fmla*,int> hit;
    this->count_fmla_refs(hit);
    map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
    Fmla* ret = this->flatten_andor_rec(hit, cache);
    stop();
    return ret;
}

Fmla* Fmla::flatten_andor_rec(std::map<Fmla*,int>& hit, std::map<Fmla*,Fmla*>& cache) {
    if (this->op == FmlaOp::VAR) {return this;}
    Fmla*& cache_entry = cache[this];
    if (cache_entry != NULL) {
        return cache_entry;
    }
    switch(this->op) {
        case FmlaOp::VAR:
            return this;
        case FmlaOp::AND:
        case FmlaOp::OR: 
        {
            vector<Fmla*> new_args;
            vector<Fmla*> arg_stack;
            for (int j = this->num_args - 1; j >= 0; j--) {
                arg_stack.push_back(this->arg[j]);
            }
            while (!arg_stack.empty()) {
                Fmla* cur_arg = arg_stack.back(); 
                arg_stack.pop_back();
                if (cur_arg->op == op && hit[cur_arg] == 1) {
                    for (int j = cur_arg->num_args - 1; j >= 0; j--) {
                        arg_stack.push_back(cur_arg->arg[j]);
                    }
                } else {
                    new_args.push_back(cur_arg->flatten_andor_rec(hit, cache));
                }
            }
            cache_entry = RawFmla<FmlaPtrVector>(op, new_args);
            return cache_entry;
        }
        default:
        {
            vector<Fmla*> new_args;
            bool unch = true;
            for (int j=0; j < this->num_args; j++) {
                Fmla* cur_arg = this->arg[j];
                Fmla* new_arg = cur_arg->flatten_andor_rec(hit, cache);
                if (cur_arg != new_arg) {unch = false;}
                new_args.push_back(new_arg);
            }
            if (unch) {
                cache_entry = this;
            } else {
                cache_entry = RawFmla<FmlaPtrVector>(op, new_args);
            }
            return cache_entry;
        }
    }
}


Fmla* Fmla::negate() {
    switch (this->op) {
        case FmlaOp::TRUE:  return fmla_false;
        case FmlaOp::FALSE: return fmla_true;
        case FmlaOp::NOT:   return this->arg[0];
        //-------------------------
        case FmlaOp::VAR: 
        case FmlaOp::AND: 
        case FmlaOp::OR: 
        case FmlaOp::ITE: 
        case FmlaOp::EQ:
        case FmlaOp::XOR: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL: 
        case FmlaOp::IMPL: 
        case FmlaOp::SUBST: 
            return RawFmla(FmlaOp::NOT, this);
        //-------------------------
        case FmlaOp::FREE: 
            return RawFmla(FmlaOp::FREE, this->arg[0], this->arg[1]->negate());
        //-------------------------
        case FmlaOp::GSEQ: 
        case FmlaOp::RESOLVE: 
        case FmlaOp::LIST: 
        case FmlaOp::INCLUDE: 
        case FmlaOp::NEWENV: 
        case FmlaOp::ERROR:
        case FmlaOp::MAX_OP:
            assert(false);
    }
    assert(false);
}

Fmla* Fmla::negate_push() {
    static map<Fmla*,Fmla*> negate_cache = map<Fmla*,Fmla*>();
    switch (this->op) {
        case FmlaOp::VAR: {
            return RawFmla(FmlaOp::NOT, this);
        }
        case FmlaOp::TRUE:  return fmla_false;
        case FmlaOp::FALSE: return fmla_true;
        case FmlaOp::NOT:   return this->arg[0];
        default:
        break;
    }
    auto(it, negate_cache.find(this));
    if (it != negate_cache.end()) {return it->second;}
    Fmla* ret;
    switch (this->op) {
        case FmlaOp::AND: 
        case FmlaOp::OR: 
        {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i]->negate_push();
            }
            FmlaOp::op ret_op = (FmlaOp::op) ((FmlaOp::AND + FmlaOp::OR) - this->op);
            ret = RawFmla<FmlaPtrVector>(ret_op, argvec);
            break;
        }
        case FmlaOp::ITE: {
            assert(this->num_args == 3);
            vector<Fmla*> argvec(3, NULL);
            argvec[0] = this->arg[0];
            argvec[1] = this->arg[1]->negate_push();
            argvec[2] = this->arg[2]->negate_push();
            ret = RawFmla<FmlaPtrVector>(FmlaOp::ITE, argvec);
            break;
        }
        case FmlaOp::EQ:
        case FmlaOp::XOR: 
        {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i];
            }
            assert(this->num_args > 0);
            argvec[0] = argvec[0]->negate_push();
            ret = RawFmla<FmlaPtrVector>(this->op, argvec);
            break;
        }
        case FmlaOp::LIST: {assert(false);}
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL: {
            FmlaOp::op ret_op = (FmlaOp::op) ((FmlaOp::EXISTS + FmlaOp::FORALL) - this->op);
            ret = RawFmla(ret_op, this->arg[0], this->arg[1]->negate_push());
            break;
        }
        case FmlaOp::FREE: {
            ret = RawFmla(FmlaOp::FREE, this->arg[0], this->arg[1]->negate_push());
            break;
        }
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
    negate_cache[this] = ret;
    return ret;
}

double Fmla::tree_size() {
    static map<Fmla*,double> cache = map<Fmla*,double>();
    switch (this->op) {
        case FmlaOp::VAR:   return 1;
        case FmlaOp::TRUE:  return 1;
        case FmlaOp::FALSE: return 1;
        case FmlaOp::NOT:   return this->arg[0]->tree_size();
        default:
        break;
    }
    auto(it, cache.find(this));
    if (it != cache.end()) {return it->second;}
    double ret = 1;
    switch (this->op) {
        case FmlaOp::AND: 
        case FmlaOp::OR: 
        case FmlaOp::ITE: 
        case FmlaOp::EQ:
        case FmlaOp::XOR: 
        {
            for (int i=0; i < this->num_args; i++) {
                ret += this->arg[i]->tree_size();
            }
            break;
        }
        case FmlaOp::LIST: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL: 
        case FmlaOp::FREE:
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
    cache[this] = ret;
    return ret;
}

int Fmla::dag_size(std::set<Fmla*>& hit) {
    switch (this->op) {
        case FmlaOp::VAR:   return 1;
        case FmlaOp::TRUE:  return 1;
        case FmlaOp::FALSE: return 1;
        case FmlaOp::NOT:   return this->arg[0]->dag_size(hit);
        default:
        break;
    }
    if (hit.count(this)) {
        return 1;
    }
    hit.insert(this);
    int ret = 1;
    switch (this->op) {
        case FmlaOp::AND: 
        case FmlaOp::OR: 
        case FmlaOp::ITE: 
        case FmlaOp::EQ:
        case FmlaOp::XOR: 
        case FmlaOp::LIST: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL: 
        case FmlaOp::FREE:
        {
            for (int i=0; i < this->num_args; i++) {
                ret += this->arg[i]->dag_size(hit);
            }
            break;
        }
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
    return ret;
}
int Fmla::dag_size() {
    set<Fmla*> hit = set<Fmla*>();
    int ret = this->dag_size(hit);
    return ret;
}

Fmla* Fmla::sort_andor_args_nonrec() {
    if (this->op != FmlaOp::AND && this->op != FmlaOp::OR) {
        return this;
    }
    vector<Fmla*> argvec(this->num_args, NULL);
    for (int i=0; i < this->num_args; i++) {
        argvec[i] = this->arg[i];
    }
    std::sort(argvec.begin(), argvec.end(), fn_lt_fmla_ptr);
    Fmla* ret = ConsFmla(this->op, argvec);
    return ret;
}

Fmla* Fmla::simp_ite() {
    static map<Fmla*,Fmla*> simp_ite_cache = map<Fmla*,Fmla*>();
    auto(it, simp_ite_cache.find(this));
    if (it != simp_ite_cache.end()) {return it->second;}
    switch (this->op) {
        case FmlaOp::VAR:
        case FmlaOp::TRUE:
        case FmlaOp::FALSE:
            return this;
        case FmlaOp::NOT: 
        case FmlaOp::AND:
        case FmlaOp::OR:
        case FmlaOp::EQ:
        case FmlaOp::XOR:
        case FmlaOp::LIST: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL:
        case FmlaOp::FREE:
        case FmlaOp::GSEQ:
        {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i]->simp_ite();
            }
            Fmla* ret = ConsFmla(this->op, argvec);
            simp_ite_cache[this] = ret;
            return ret;
        }
        case FmlaOp::ITE: {
            assert(this->num_args == 3);
            vector<Fmla*> argvec(3, NULL);
            Fmla* test = this->arg[0]->simp_ite();
            Fmla* tbra = this->arg[1]->simp_ite();
            Fmla* fbra = this->arg[2]->simp_ite();
            Fmla* ret;
            if (0) {}
            else if (tbra == fmla_true)  {ret = ConsFmla(FmlaOp::OR,  fbra, test);}
            else if (fbra == fmla_true)  {ret = ConsFmla(FmlaOp::OR,  tbra, test->negate());}
            else if (tbra == fmla_false) {ret = ConsFmla(FmlaOp::AND, fbra, test->negate());}
            else if (fbra == fmla_false) {ret = ConsFmla(FmlaOp::AND, tbra, test);}
            else                         {ret = ConsFmla(this->op, test, tbra, fbra);}
            ret = ret->sort_andor_args_nonrec();
            simp_ite_cache[this] = ret;
            return ret;
        }
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
}

Fmla* Fmla::to_nnf() {
    static map<Fmla*,Fmla*> to_nnf_cache = map<Fmla*,Fmla*>();
    auto(it, to_nnf_cache.find(this));
    if (it != to_nnf_cache.end()) {return it->second;}
    Fmla* ret;
    switch (this->op) {
        case FmlaOp::VAR:
        case FmlaOp::TRUE:
        case FmlaOp::FALSE:
            return this;
        case FmlaOp::AND:
        case FmlaOp::OR:
        case FmlaOp::LIST: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL:
        case FmlaOp::FREE:
        {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i]->to_nnf();
            }
            ret = ConsFmla(this->op, argvec);
            break;
        }
        case FmlaOp::NOT: {
            return this->arg[0]->to_nnf()->negate_push();
        }
        case FmlaOp::ITE: {
            assert(this->num_args == 3);
            vector<Fmla*> argvec(3, NULL);
            Fmla* test = this->arg[0]->to_nnf();
            Fmla* tbra = this->arg[1]->to_nnf();
            Fmla* fbra = this->arg[2]->to_nnf();
            ret = RawFmla(FmlaOp::OR, 
                    ConsFmla(FmlaOp::AND, FmlaPtrVec(test, tbra)),
                    ConsFmla(FmlaOp::AND, FmlaPtrVec(test->negate_push(), fbra)));
            break;
        }
        case FmlaOp::EQ: {
            assert(this->num_args == 2);
            Fmla* a1 = this->arg[0]->to_nnf();
            Fmla* a2 = this->arg[1]->to_nnf();
            // ret = ConsFmla(FmlaOp::OR, 
            //         ConsFmla(FmlaOp::AND, a1, a2),
            //         ConsFmla(FmlaOp::AND, a1->negate_push(), a2->negate_push()));
            ret = ConsFmla(FmlaOp::AND, 
                    ConsFmla(FmlaOp::OR, a1, a2->negate_push()),
                    ConsFmla(FmlaOp::OR, a1->negate_push(), a2));
            break;
        }
        case FmlaOp::XOR: {
            ret = fmla_false;
            for (int i=0; i < this->num_args; i++) {
                Fmla* cur_arg = this->arg[i]->to_nnf();
                ret = ConsFmla(FmlaOp::OR,
                        ConsFmla(FmlaOp::AND, ret, cur_arg->negate_push()),
                        ConsFmla(FmlaOp::AND, ret->negate_push(), cur_arg));
            }
            break;
        }
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
    to_nnf_cache[this] = ret;
    return ret;
}

Fmla* Fmla::nnf_to_aig() {
    static map<Fmla*,Fmla*> to_aig_cache = map<Fmla*,Fmla*>();
    auto(it, to_aig_cache.find(this));
    if (it != to_aig_cache.end()) {return it->second;}
    Fmla* ret;
    switch (this->op) {
        case FmlaOp::VAR:
        case FmlaOp::TRUE:
        case FmlaOp::FALSE:
            return this;
        case FmlaOp::NOT: {
            assert(this->arg[0]->op == FmlaOp::VAR);
            return this;
        }
        case FmlaOp::AND:
        case FmlaOp::LIST: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL:
        case FmlaOp::FREE:
        {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i]->nnf_to_aig();
            }
            ret = ConsFmla(this->op, argvec);
            break;
        }
        case FmlaOp::OR: {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i]->nnf_to_aig()->negate();
            }
            ret = ConsFmla(FmlaOp::AND, argvec)->negate();
            break;
        }
        default:
            assert(false);
    }
    to_aig_cache[this] = ret;
    return ret;
}


/*void merge_lists(Fmla* L1, Fmla* L2, vector<Fmla*>& result) {
    assert(result.size() == 0);
    result.reserve(L1->num_args + L2->num_args);
    for (int i=0; i < L1->num_args; i++) {result.push_back(L1->arg[i]);}
    for (int i=0; i < L2->num_args; i++) {result.push_back(L2->arg[i]);}
    std::sort(result.begin(), result.end());
    int size = std::unique(result.begin(), result.end()) - result.begin();
    result.resize(size);
}*/

int merge_resol_lists(vector<Fmla*>& L1, Fmla* L2, vector<Fmla*>& result, Fmla* resolvent) {
    assert(result.size() == 0);
    Fmla* neg_resol = (resolvent==NULL ? NULL : resolvent->negate());
    result.reserve(L1.size() + L2->num_args + (resolvent ? 0 : 2));
    bool has_resol = (resolvent==NULL);
    for (int i=0; i < (int) L1.size(); i++) {
        if (L1[i] != resolvent) {result.push_back(L1[i]);}
        else {has_resol = true;}
    }
    if (!has_resol) {return -1;}
    has_resol = (resolvent==NULL);
    for (int i=0; i < L2->num_args; i++) {
        if (L2->arg[i] != neg_resol) {result.push_back(L2->arg[i]);}
        else {has_resol = true;}
    }
    if (!has_resol) {return -2;}
    std::sort(result.begin(), result.end(), fn_lt_fmla_ptr);
    int size = std::unique(result.begin(), result.end()) - result.begin();
    result.resize(size);
    return 0;
}

bool Fmla::is_quant() {
    return FmlaOp::is_quant(this->op);
}

bool Fmla::is_lit() {
    switch(this->op) {
        case FmlaOp::VAR:
            return true;
        case FmlaOp::NOT:
            return (this->arg[0]->op == FmlaOp::VAR);
        default:
            return false;
    }
}

Fmla* Fmla::var_of_lit() {
    switch(this->op) {
        case FmlaOp::VAR:
            return this;
        case FmlaOp::NOT:
            return this->arg[0];
        default:
            return fmla_error;
    }
}

const char* Fmla::name_of_var() {
    if (this->op == FmlaOp::VAR) {
        return (const char*)(this->arg[0]);
    } else {
        die_f("Expected a variable, but found a %s!", 
            FmlaOp::enum_to_str(this->op));
    }
}

Fmla* Fmla::eval_resolve(std::map<Fmla*, Fmla*>& cache) {
    switch (this->op) {
        case FmlaOp::VAR:
        case FmlaOp::TRUE:
        case FmlaOp::FALSE:
        case FmlaOp::AND:
        case FmlaOp::OR:
        case FmlaOp::NOT: 
        case FmlaOp::ITE: 
        case FmlaOp::EQ: 
        case FmlaOp::XOR: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL:
        case FmlaOp::GSEQ:
            return this;
        case FmlaOp::FREE:
        case FmlaOp::LIST: 
        {
            vector<Fmla*> argvec(this->num_args, NULL);
            for (int i=0; i < this->num_args; i++) {
                argvec[i] = this->arg[i]->eval_resolve(cache);
            }
            return ConsFmla(this->op, argvec);
        }
        case FmlaOp::RESOLVE: 
        {
            if (cache.count(this)) {
                return cache[this];
            }
            Fmla* gs1 = this->arg[0]->eval_resolve(cache);
            Fmla* gs1_Lnow = gs1->arg[0];
            Fmla* gs1_Lfut = gs1->arg[1];
            Fmla* resolv_list = this->arg[1];
            assert(resolv_list->op == FmlaOp::LIST);
            Fmla* ans = gs1->arg[2];
            vector<Fmla*>* cur_Lnow = new vector<Fmla*>(&gs1_Lnow->arg[0], 
                    &gs1_Lnow->arg[gs1_Lnow->num_args]);
            vector<Fmla*>* cur_Lfut = new vector<Fmla*>(&gs1_Lfut->arg[0], 
                    &gs1_Lfut->arg[gs1_Lfut->num_args]);
            for (int i = 0; i < resolv_list->num_args; i++) {
                Fmla* resolvent = resolv_list->arg[i]->arg[0];
                Fmla* resol_var = resolvent->var_of_lit();
                if (resol_var->op != FmlaOp::VAR) {
                    fprintf(stderr, "Error: resolvent is not a literal!\n");
                    abort();
                }
                Fmla* gs2 = resolv_list->arg[i]->arg[1]->eval_resolve(cache);
                assert(gs2->op == FmlaOp::GSEQ);
                vector<Fmla*>* old_Lnow = cur_Lnow;
                vector<Fmla*>* old_Lfut = cur_Lfut;
                cur_Lnow = new vector<Fmla*>();
                cur_Lfut = new vector<Fmla*>();
                merge_resol_lists(*old_Lfut, gs2->arg[1], *cur_Lfut, NULL);
                int err = merge_resol_lists(*old_Lnow, gs2->arg[0], *cur_Lnow, resolvent);
                if (err) {
                    fprintf(stderr, "Error: bad resolvent!\n"); abort();
                }
                FmlaOp::op resol_type = resolv_list->arg[i]->op;
                if (resol_type == FmlaOp::FREE) {
                    ans = ConsFmla(FmlaOp::ITE, resolvent, ans, gs2->arg[2])->simp_ite();
                } else if (resol_type == FmlaOp::EXISTS) {
                    ans = ConsFmla(FmlaOp::OR, ans, gs2->arg[2])->simp_ite();
                    if (ans != gs2->arg[2]) {
                        cur_Lfut->push_back(resol_var);
                        cur_Lfut->push_back(resol_var->negate());
                    }
                } else if (resol_type == FmlaOp::FORALL) {
                    ans = ConsFmla(FmlaOp::AND, ans, gs2->arg[2])->simp_ite();
                    if (ans != gs2->arg[2]) {
                        cur_Lfut->push_back(resol_var);
                        cur_Lfut->push_back(resol_var->negate());
                    }
                } else {
                    fprintf(stderr, "Error: bad resolve step!\n");
                    abort();
                }
                delete old_Lnow;
                delete old_Lfut;
            }
            Fmla* ret = ConsFmla(FmlaOp::GSEQ, 
                ConsFmla(FmlaOp::LIST, *cur_Lnow),
                ConsFmla(FmlaOp::LIST, *cur_Lfut),
                ans);
            cache[this] = ret;
            return ret;
        }
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
}

std::map<Fmla*,Fmla*>* Fmla::litset_to_varmap() {
    map<Fmla*,Fmla*>* ret = new map<Fmla*,Fmla*>();
    assert(this->op == FmlaOp::LIST);
    for (int i=0; i < this->num_args; i++) {
        Fmla* val = fmla_true;
        Fmla* cur = this->arg[i];
        if (cur->op == FmlaOp::NOT) {
            val = fmla_false;
            cur = cur->arg[0];
        }
        assert(cur->op == FmlaOp::VAR);
        (*ret)[cur] = val;
    }
    return ret;
}

Fmla* Fmla::subst_asgn(Fmla* asgn) {
    map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
    map<Fmla*,Fmla*>* varmap = asgn->litset_to_varmap();
    Fmla* ret = this->subst(*varmap, cache);
    delete varmap;
    return ret;
}

Fmla* Fmla::subst(std::map<Fmla*,Fmla*>& rep, std::map<Fmla*,Fmla*>& cache) {
    {
        auto(it, rep.find(this));
        if (it != rep.end()) {
            return it->second;
        }
    }
    switch (this->op) {
        case FmlaOp::VAR: {
            return this;
        }
        case FmlaOp::EXISTS:
        case FmlaOp::FORALL:
        case FmlaOp::FREE:
            return ConsFmla(this->op, this->arg[0], this->arg[1]->subst(rep, cache));
        default:;
    }
    {
        auto(it, cache.find(this));
        if (it != cache.end()) {
            return it->second;
        }
    }
    vector<Fmla*> args;
    args = vector<Fmla*>(&this->arg[0], &this->arg[this->num_args]);
    foreach (it_arg, args) {
        *it_arg = (*it_arg)->subst(rep, cache);
    }
    Fmla* ret = ConsFmla(this->op, args);
    cache[this] = ret;
    return ret;
}

Fmla* Fmla::subst_one(Fmla* var, Fmla* val) {
    map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
    map<Fmla*,Fmla*> rep = map<Fmla*,Fmla*>();
    rep[var] = val;
    return this->subst(rep, cache);
}

void Fmla::find_vars_in_fmla(std::set<Fmla*>* vars) {
    set<Fmla*>* hit = new set<Fmla*>();
    this->find_vars_in_fmla(vars, hit);
    delete hit;
}

void Fmla::find_vars_in_fmla(std::set<Fmla*>* vars, std::set<Fmla*>* hit) {
    if (hit->count(this)) {return;}
    hit->insert(this);
    switch (this->op) {
        case FmlaOp::VAR:
            vars->insert(this);
            return;
        case FmlaOp::TRUE:
        case FmlaOp::FALSE:
            return;
        case FmlaOp::AND:
        case FmlaOp::OR:
        case FmlaOp::NOT: 
        case FmlaOp::ITE: 
        case FmlaOp::EQ: 
        case FmlaOp::XOR: 
        case FmlaOp::LIST: 
        case FmlaOp::EXISTS: 
        case FmlaOp::FORALL:
        case FmlaOp::FREE:
        case FmlaOp::GSEQ:
        {
            for (int i=0; i < this->num_args; i++) {
                this->arg[i]->find_vars_in_fmla(vars, hit);
            }
            return;
        }
        case FmlaOp::ERROR:
        default:
            assert(false);
    }
    
}

void Fmla::count_fmla_refs(std::map<Fmla*, int>& hit) {
    int& c = hit[this];
    if (c > 0) {
        c++;
        return;
    }
    c++;
    switch (this->op) {
        case FmlaOp::VAR:
            return;
        default:
        {
            for (int i=0; i < this->num_args; i++) {
                this->arg[i]->count_fmla_refs(hit);
            }
            return;
        }
    }
}


// struct var_order_by_id {
//     inline Fmla*& operator[](int i) {
//         return i;
//     }
// };
    

Fmla* Fmla::to_bdd(std::vector<Fmla*>& var_order, int var_pos, std::map<Fmla*,Fmla*>& cache) {
    if (this == fmla_true || this == fmla_false) {
        return this;
    }
    switch (this->op) {
        case FmlaOp::FREE:
        case FmlaOp::EXISTS:
        case FmlaOp::FORALL:
            return ConsFmla(this->op, this->arg[0], 
                this->arg[1]->to_bdd(var_order, var_pos, cache));
        case FmlaOp::GSEQ:
            return ConsFmla(this->op, this->arg[0], this->arg[1], 
                this->arg[2]->to_bdd(var_order, var_pos, cache));
        case FmlaOp::LIST:
            {
            vector<Fmla*> new_args;
            new_args.reserve(this->num_args);
            for (int i=0; i < this->num_args; i++) {
                new_args.push_back(this->arg[i]->to_bdd(var_order, var_pos, cache));
            }
            return ConsFmla(this->op, new_args);
            }
        default: 
            break;
    }
    auto(itAns, cache.find(this));
    if (itAns != cache.end()) {return itAns->second;}
    assert(var_pos < (int) var_order.size());
    Fmla* cur_var = var_order[var_pos];
    Fmla* tbra = this->subst_one(cur_var, fmla_true)->to_bdd(var_order, var_pos+1, cache);
    Fmla* fbra = this->subst_one(cur_var, fmla_false)->to_bdd(var_order, var_pos+1, cache);
    Fmla* ret = ConsFmla(FmlaOp::ITE, cur_var, tbra, fbra);
    cache[this] = ret;
    return ret;
}

Fmla* Fmla::to_bdd(Fmla* given_var_order) {
    set<Fmla*>* var_set = new set<Fmla*>();
    this->find_vars_in_fmla(var_set);
    vector<Fmla*> var_vec;
    set<Fmla*> seen;
    if (given_var_order) {
        for (int i=0; i < given_var_order->num_args; i++) {
            Fmla* cur_var = given_var_order->arg[i];
            if (cur_var->op != FmlaOp::VAR) {
                fprintf(stderr, "Warning: bdd_order[%i] is not a variable!\n", i);
                continue;
            }
            if (seen.count(cur_var) == 0) {
                seen.insert(cur_var);
                var_vec.push_back(cur_var);
            }
        }
    }
    for (set<Fmla*>::iterator it = var_set->begin(); it != var_set->end(); ++it) {
        Fmla* cur_var = *it;
        if (seen.count(cur_var) == 0) {
            seen.insert(cur_var);
            var_vec.push_back(cur_var);
        }
    }
    map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
    return this->to_bdd(var_vec, 0, cache);
}

/****************************************************************************/

bool glo_qcir_ren = false;

static void DieParse(const char* pErrMsg, FmlaLexer* lex, ...) {
    fprintf(stderr, "Error on line %i, col %i: ", lex->LineNum, lex->CurPos);
    va_list ap;
    va_start(ap, lex);
    vfprintf(stderr, pErrMsg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    abort();
}

class FmlaParser {
    public:
    FmlaLexer& lex;
    map<sym, Fmla*, lt_str> gate_by_name;

    FmlaParser(FmlaLexer* p_lex): lex(*p_lex), gate_by_name() { }
    Fmla* parse();
};

static set<const char*, lt_str> fmla_parser_opened_files;

Fmla* parse_fmla_file(const char* filename) {
    Fmla* ret;
    if (fmla_parser_opened_files.count(filename)) {
        const char* s = StrFmt("File '%.72s' is already open for parsing!\n", filename);
        return RawFmla(FmlaOp::ERROR, FmlaVar(s));
    }
    fmla_parser_opened_files.insert(filename);
    FmlaLexer* lex = FmlaLexer::new_from_file(filename);
    if (StrEqc(lex->CurTok.p, "(")) {
        char buf[80];
        sscanf(lex->CurLine.p, "%20s", buf);
        if (StrEqc(buf, "(FmlaBin)")) {
            FILE* file = lex->infile;
            //FILE* file = fopen(filename, "r");
            rewind(file);
            ret = FmlaBinReader(file).read_file();
            delete lex; // This closes the file.
            //fclose(file);
            return ret;
        }
    }
    ret = FmlaParser(lex).parse();
    delete lex;
    fmla_parser_opened_files.erase(filename);
    return ret;
}

Fmla* read_qcir_var(FmlaLexer* lex) {
    assert(lex->CurTok.p != NULL);
    switch (lex->CurTok.p[0]) { 
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '0' ... '9':
        case '_':
            break;
        default:
            DieParse("Error: Expecting an alphanumeric but found '%s'.", lex, lex->CurTok.p);
    }
    Fmla* ret = FmlaVar(lex->CurTok.p);
    lex->AdvTok();
    return ret;
}

Fmla* read_qcir_lit(FmlaLexer* lex) {
    if (StrEqc(lex->CurTok.p, "-")) {
        lex->AdvTok();
        return read_qcir_var(lex)->negate();
    } else {
        return read_qcir_var(lex);
    }
}

vector<Fmla*> read_qcir_arglist(FmlaLexer* lex) {
    vector<Fmla*> ret = vector<Fmla*>();
    lex->EatTok("(");
    while (true) {
        if (StrEqc(lex->CurTok.p, ")")) {
            lex->AdvTok();
            break;
        }
        if (StrEqc(lex->CurTok.p, ";")) {
            lex->AdvTok();
            Fmla* qvars = RawFmla(FmlaOp::LIST, ret);
            ret.clear();
            ret.push_back(qvars);
        }
        ret.push_back(read_qcir_lit(lex));
    }
    return ret;
}

Fmla* parse_qcir_file(const char* filename) {
    FmlaLexer* lex = new FmlaLexer();
    lex->SaveComments = true;
    lex->init_from_file(filename);
    map<sym, sym, lt_str> rename_table = map<sym, sym, lt_str>();
    while (true) {
        if (lex->CurTok.p != NULL && lex->CurTok.p[0] == '#') {
            if (glo_qcir_ren) {
                char num_name[60+1];
                char orig_name[60+1];
                if (2 == sscanf(lex->CurTok.p, "#VarName %60s : %60s", num_name, orig_name)) {
                    //printf("%s\n", lex->CurTok.p);
                    rename_table[SymTab[num_name]] = SymTab[orig_name];
                }
            }
            lex->AdvTok();
        } else {
            break;
        }
    }
    lex->SaveComments = false;
    vector<pair<sym, vector<Fmla*> > > qblocks = vector<pair<sym, vector<Fmla*> > >();
    set<Fmla*> qvar_set = set<Fmla*>();
    while (true) {
        bool is_q = (
            StrEqc(lex->CurTok.p, "exists") ||
            StrEqc(lex->CurTok.p, "forall") ||
            StrEqc(lex->CurTok.p, "free"));
        if (is_q) {
            const char* q_op = read_qcir_var(lex)->name_of_var();
            vector<Fmla*> args = read_qcir_arglist(lex);
            foreach (it, args) {
                Fmla* arg = *it;
                if (arg->op != FmlaOp::VAR) {
                    DieParse("Error: Expecting a variable but found a negative literal.", lex);
                }
                const char* arg_name = arg->name_of_var();
                if (rename_table.count(arg_name) == 1) {
                    *it = FmlaVar(rename_table[arg_name]);
                }
                qvar_set.insert(*it);
            }
            qblocks.push_back(pair<sym, vector<Fmla*> >(q_op, args));
        } else {
            break;
        }
    }
    lex->EatTok("output");
    vector<Fmla*> output_list = read_qcir_arglist(lex);
    if (output_list.size() != 1) {
         DieParse("Error: Expecting exactly 1 output, but found %s.", lex, output_list.size());
    }
    Fmla* out_gate = output_list[0];

    map<sym, Fmla*, lt_str> gate_table = map<sym, Fmla*, lt_str>();
    set<sym, lt_str> undef_vars = set<sym, lt_str>();
    while (true) {
        if (lex->IsEOF()) {
            break;
        }
        const char* gate_name = read_qcir_var(lex)->name_of_var();
        lex->EatTok("=");
        const char* op_name = read_qcir_var(lex)->name_of_var();
        vector<Fmla*> args = read_qcir_arglist(lex);
        foreach (it, args) {
            Fmla* arg = *it;
            if (arg->op == FmlaOp::LIST) {
                if (glo_qcir_ren) {
                    DieParse("Error: Cannot use option '-qcir-ren-tab' on non-prenex files.", lex, output_list.size());
                }
                Fmla** qvars = arg->arg;
                for (int i=0; i < arg->num_args; i++) {
                    if (qvars[i]->op != FmlaOp::VAR) {
                        fprintf(stderr, "Error: Not a variable: ");
                        qvars[i]->write();
                        abort();
                    }
                    qvar_set.insert(qvars[i]);
                    undef_vars.erase(qvars[i]->name_of_var());
                }
                continue;
            }
            const char* arg_name = arg->var_of_lit()->name_of_var();
            if (gate_table.count(arg_name) == 1) {
                *it = gate_table[arg_name];
                if (arg->op == FmlaOp::NOT) {
                    *it = (*it)->negate();
                }
            } else {
                if (rename_table.count(arg_name) == 1) {
                    *it = FmlaVar(rename_table[arg_name]);
                    if (arg->op == FmlaOp::NOT) {
                        *it = (*it)->negate();
                    }
                }
                if (qvar_set.count((*it)->var_of_lit()) == 0) {
                    undef_vars.insert((*it)->var_of_lit()->name_of_var());
                }
            }
        }
        gate_table[gate_name] = ConsFmla(op_name, args);
    }
    if (undef_vars.size() > 0) {
        fprintf(stderr, "ERROR: Undefined variables/gates:");
        foreach(it, undef_vars) {
            fprintf(stderr, " %s", *it);
        }
        fprintf(stderr, "\n");
    }
    Fmla* out_fmla;
    if (out_gate->op == FmlaOp::NOT) {
        out_fmla = gate_table[out_gate->negate()->name_of_var()]->negate();
    } else {
        out_fmla = gate_table[out_gate->name_of_var()];
    }
    std::reverse(qblocks.begin(), qblocks.end());
    foreach (it, qblocks) {
        FmlaOp::op q_op = FmlaOp::str_to_enum(it->first);
        vector<Fmla*> qvars = it->second;
        out_fmla = ConsFmla(q_op, ConsFmla(FmlaOp::LIST, qvars), out_fmla);
    }
    return out_fmla;
}

Fmla* Fmla::parse_str(const char* str) /*NO_PROTO*/ {
    FmlaLexer* lex = new FmlaLexer();
    lex->init_from_str(str);
    Fmla* ret = FmlaParser(lex).parse();
    delete lex;
    return ret;
}

int req_num_args_for_op(FmlaOp::op op) {
    switch (op) {
        case FmlaOp::ITE:
        case FmlaOp::GSEQ:
            return 3;
        case FmlaOp::FORALL:
        case FmlaOp::EXISTS:
        case FmlaOp::FREE:
            return 2;
        default:
            return -1;
    }
}

Fmla* FmlaParser::parse() {
    if (lex.CurTok.p == NULL) {
        return NULL;
    }
    if (lex.CurTok.n == 1 && lex.CurTok.p[0] == '-') {
        lex.EatTok("-");
        return parse()->negate();
    }
    sym gate_name = NULL;
    FmlaOp::op op = FmlaOp::ERROR;
    vector<Fmla*> argvec = vector<Fmla*>();
    int StartLine = lex.LineNum;
    int StartCol = lex.CurPos;
    char open_paren = '(';
    char close_paren = ')';
    char ic = (lex.CurTok.n >= 1 ? lex.CurTok.p[0] : 0); 
    if (ic == ')' || ic == ']' || ic == ':' || ic == '(') {
        DieParse("Unexpected '%c'.", &lex, ic);
    } else if (ic == '[') {
        op = FmlaOp::LIST;
        lex.EatTok("[");
        open_paren = '[';
        close_paren = ']';
    } else {
        if (lex.peek() == ':') {
            gate_name = SymTab[lex.CurTok.p];
            lex.AdvTok();
            lex.EatTok(":");
        }
        if (lex.peek() == '(') {
            op = FmlaOp::str_to_enum(lex.CurTok.p);
            if (op == FmlaOp::ERROR) {
                DieParse("Invalid operator: '%s'.", &lex, lex.CurTok.p);
            }
            lex.AdvTok();
            lex.EatTok("(");
        } else {
            sym s = SymTab[lex.CurTok.p];
            Fmla* ret;
            if (strlen(s) > 0 && s[0] == '$') {
                ret = this->gate_by_name[SymTab[s]];
                if (ret == NULL) {  
                    DieParse("Undefined subformula name: '%s'.", &lex, s);
                }
            } else {
                if (StrEqc(s, "true") || StrEqc(s, "false")) {
                    DieParse("Invalid variable name: '%s'.", &lex, s);
                }
                argvec.push_back((Fmla*) s);
                ret = RawFmla<FmlaPtrVector>(FmlaOp::VAR, argvec);
            }
            lex.AdvTok();
            if (lex.CurTok.n == 1 && lex.CurTok.p[0] == '(') {
                DieParse("Extraneous whitespace before '%s'.", &lex, lex.CurTok.p);
            }
            return ret;
        }
    }
    while (true) {
        if (lex.CurTok.n == 1 && lex.CurTok.p[0] == close_paren) {
            lex.AdvTok();
            break;
        }
        Fmla* arg;
        if (op != FmlaOp::NEWENV) {
            arg = parse();
        } else {
            arg = FmlaParser(&this->lex).parse();
        }
        if (arg == NULL) {
            DieParse("Unexpected end of file: trying to match '%c' on line %i, column %i.", 
                &lex, open_paren, StartLine, StartCol);
        }
        argvec.push_back(arg);
    }
    int req_num_args = req_num_args_for_op(op);
    if (req_num_args != -1 && (int)argvec.size() != req_num_args) {
        fprintf(stderr, "Line %i, col %i: "
            "The '%s' operator requires exactly %i args, but %i given.\n",
            StartLine, StartCol, FmlaOp::enum_to_str(op), 
            req_num_args, (int)argvec.size());
        exit(1);
    }
    Fmla* ret = ConsFmla(op, argvec);
    if (op == FmlaOp::INCLUDE && ret->op == FmlaOp::ERROR) {
        fprintf(stderr, "Line %i, col %i: Error occurred when trying to include file.\n",
            StartLine, StartCol);
        if (ret->num_args == 1 && ret->arg[0]->op == FmlaOp::VAR) {
            const char* msg = ret->arg[0]->name_of_var();
            fprintf(stderr, "%s", msg);
        }
        exit(1);
    }
    if (gate_name) {
        if (this->gate_by_name.count(gate_name)) {
            fprintf(stderr, "Line %i, col %i: "
                "Subformula name '%s' is defined more than once.\n",
                StartLine, StartCol, gate_name);
            exit(1);
        }
        this->gate_by_name[gate_name] = ret;
    }
    return ret;
}

struct FmlaWriteInfo {
    int hit;
    int size;
    int name;
};

struct FmlaWriterOpts {
    FILE* out;
    int cur_col;
    int cur_indent;
    int cur_line;
    int cur_name;
    int max_col;
    int max_indent;
    map<Fmla*, FmlaWriteInfo>* fmla_info;
    FmlaWriterOpts():
        cur_col(0), cur_indent(0), cur_line(0), cur_name(0), max_col(79), max_indent(40), fmla_info(NULL) {}
};

void Fmla::find_repr_sizes(FmlaWriterOpts& opts) {
    FILE* dev_null = fopen("/dev/null", "w");
    FILE* old_out = opts.out;
    opts.out = dev_null;
    this->write_rec(opts, 0);
    fclose(dev_null);
    opts.out = old_out;
    opts.cur_col = opts.cur_indent = opts.cur_line = 0;
    foreach (it, *opts.fmla_info) {
        it->second.hit = 0;
    }
}

void Fmla::write(FILE* out) {
    FmlaWriterOpts opts = FmlaWriterOpts();
    opts.out = out;
    this->write(opts);
}

void Fmla::write(FmlaWriterOpts opts) {
    assert(opts.fmla_info == NULL);
    opts.fmla_info = new map<Fmla*, FmlaWriteInfo>();
    this->find_repr_sizes(opts);
    this->write_rec(opts, 0);
    delete opts.fmla_info;
}

extern "C" void write_fmla(Fmla* fmla, FILE* out) {
    fmla->write(out);
}
    

void dump_fmla(Fmla* fmla) {
    fmla->write(stderr);
    fprintf(stderr, "\n");
}
void Fmla::write() {
    this->write(stderr);
    fprintf(stderr, "\n");
}

#define adj_indent(indent) min(indent, opts.max_indent)

int Fmla::write_rec(FmlaWriterOpts& opts, int indent) {
    FILE* out = opts.out;
    int& cur_col = opts.cur_col;
    FmlaWriteInfo& this_info = (*opts.fmla_info)[this];
    int& this_size = this_info.size;
    int adj_size = this_size;
    if (adj_size == 0) {adj_size = 4;}
    int& hit_this = this_info.hit;
    if (this->op != FmlaOp::VAR && hit_this > 0) {
        adj_size = 5;
    }
    if (cur_col > opts.max_col - adj_size && cur_col > adj_indent(indent)) {
        cur_col = adj_indent(indent);
        opts.cur_indent = indent;
        fprintf(out, "\n%*s", adj_indent(indent), "");
        opts.cur_line++;
    }
    const char* name = NULL;
    int name_len = 0;
    if (this->op == FmlaOp::TRUE) {name = "true()";}
    if (this->op == FmlaOp::FALSE) {name = "false()";}
    if (this->op == FmlaOp::ERROR) {name = "error()";}
    if (this->op == FmlaOp::VAR) {
        name = (char*) this->arg[0];
    }
    if (this->op == FmlaOp::LIST && this->num_args == 0) {
        name = (char*) "[]";
    }
    if (name) {
        name_len = strlen(name);
        this_size = name_len;
        fprintf(out, "%s", name);
        cur_col += name_len;
        return name_len;
    }
    if (hit_this > 0) {
        int ret;
        if (this_info.name < 0) {
            this_info.name = -this_info.name;
        }
        fprintf(out, "$%i%n", this_info.name, &ret);
        cur_col += ret;
        return ret;
    }
    if (!(op == FmlaOp::EXISTS || op == FmlaOp::FORALL || op == FmlaOp::FREE)) {
        if (indent < (opts.max_indent / 2)) {
            indent += 2;
        } else {
            indent += 1;
        }
    }
    int cur_len;
    int tot_len = 0;
    const char* closing = ")";
    if (this->op != FmlaOp::NOT) {
        hit_this++;
        if (this_size == 0 || this_info.name > 0) {
            if (this_info.name == 0) {
                this_info.name = -(++opts.cur_name);
                assert(this_size == 0);
            }
            fprintf(out, "$%i:%s(%n", this_info.name, FmlaOp::enum_to_str(this->op), &cur_len);
        } else {
            fprintf(out, "%s(%n", FmlaOp::enum_to_str(this->op), &cur_len);
        }
    } else {
        assert(this->num_args > 0);
        if (this->arg[0]->op == FmlaOp::VAR) {
            fprintf(out, "-%n", &cur_len);
            if (cur_len + cur_col >= opts.max_col) {
                cur_len = 0;
            }
            closing = "";
        } else {
            fprintf(out, "%s(%n", FmlaOp::enum_to_str(this->op), &cur_len);
        }
    }
    tot_len += cur_len;
    cur_col += cur_len;
    for (int i=0; i < this->num_args; i++) {
        cur_len = this->arg[i]->write_rec(opts, indent);
        tot_len += cur_len;
        if (i < this->num_args - 1) {
            fprintf(out, ", ");
            tot_len += 2;
            cur_col += 2;
            if (opts.cur_indent > indent) {
                cur_col = adj_indent(indent);
                opts.cur_indent = indent;
                fprintf(out, "\n%*s", adj_indent(indent), "");
                opts.cur_line++;
            }
        }
    }
    fprintf(out, "%s", closing);
    int len_closing = strlen(closing);
    tot_len += len_closing;
    cur_col += len_closing;
    this_size = tot_len; 
    return tot_len;
}


bool Fmla::match_pat(Fmla* pat, std::vector<Fmla*>* result) {
    Fmla* subject = this;
    if (pat->op == FmlaOp::VAR) {
        char* pat_name = (char*) pat->arg[0];
        if (pat_name[0] == '\0' || pat_name[1] != '\0') {
            die_f("Bad pattern: Variable name must be a single digit.\n");
        }
        int pat_num = pat_name[0] - '0';
        if (!(0 <= pat_num && pat_num <= 9)) {
            die_f("Bad pattern: Variable name must be a single digit.\n");
        }
        Fmla** ret_slot = &result->at(pat_num);
        // if (*ret_slot != NULL) {
        //     die_f("Bad pattern: Two slots with same number or bad result array.\n");
        // }
        *ret_slot = subject;
        return true;
    }
    if (pat->op != subject->op) {
        return false;
    }
    if (pat->num_args != subject->num_args) {
        return false;
    }
    for (int i = 0; i < pat->num_args; i++) {
        bool ok = subject->arg[i]->match_pat(pat->arg[i], result);
        if (!ok) {return false;}
    }
    return true;
}

Fmla* Fmla::find_ites_in_aig() {
    vector<Fmla*> m(4, NULL);
    map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
    return find_ites_in_aig_rec(m, cache);
}

Fmla* Fmla::find_ites_in_aig_rec(std::vector<Fmla*>& m, std::map<Fmla*,Fmla*>& cache) {
    /* ite(i,t,e) = or(and(i,t), and(-i,e))
     *            = and(or(-i,t), or(i,e))
     *            = and(-and(i,-t), -and(-i,-e)) */
    static Fmla* pat = NULL;
    static const char* pat_str = "and(-and(0,1), -and(2,3))";
    if (this->op == FmlaOp::VAR) {
        return this;
    }
    Fmla*& cache_entry = cache[this];
    if (cache_entry != NULL) {
        return cache_entry;
    }
    if (pat == NULL) {
        pat = Fmla::parse_str(pat_str);
    }
    // m[0] = NULL;
    // m[1] = NULL;
    // m[2] = NULL;
    // m[3] = NULL;
    if (this->match_pat(pat, &m)) {
        Fmla* e_i = NULL;
        Fmla* e_t = NULL;
        Fmla* e_e = NULL;
        for (int p_i = 0; p_i <= 1; p_i++) {
            for (int p_ni = 0; p_ni <= 1; p_ni++) {
                if (m[p_i] == m[2+p_ni]->negate()) {
                    e_i = m[p_i];
                    e_t = m[1-p_i]->negate();
                    e_e = m[2+(1-p_ni)]->negate();
                    if (e_i->is_lit()) {
                        goto done_looking;
                    }
                }
            }
        }
        done_looking:
        if (e_i != NULL) {
            e_i = e_i->find_ites_in_aig_rec(m, cache);
            e_t = e_t->find_ites_in_aig_rec(m, cache);
            e_e = e_e->find_ites_in_aig_rec(m, cache);
            if (e_t == e_e->negate()) {
                cache_entry = ConsFmla(FmlaOp::XOR, e_i, e_e);
            } else {
                cache_entry = ConsFmla(FmlaOp::ITE, e_i, e_t, e_e);
            }
            return cache_entry;
        }
    }
    vector<Fmla*> new_args;
    new_args.reserve(this->num_args);
    bool modified = false;
    for (int i=0; i < this->num_args; i++) {
        Fmla* new_arg = this->arg[i]->find_ites_in_aig_rec(m, cache);
        if (new_arg != this->arg[i]) {modified = true;}
        new_args.push_back(new_arg);
    }
    if (!modified) {
        cache_entry = this;
    } else {
        cache_entry = ConsFmla(this->op, new_args);
    }
    return cache_entry;
}



/************************************************************************/

FmlaBinWriter::FmlaBinWriter(FILE* _file) {
    this->file = _file;
    this->comment = "";
    this->op_rewrite = talloc(int, FmlaOp::MAX_OP + 1);
    this->strip_symtab = false;
}
FmlaBinWriter::~FmlaBinWriter() {
    free(this->op_rewrite);
}


void FmlaBinWriter::write_int(unsigned int x) {
    unsigned char ch;
    while (x & ~0x7f) {
        ch = (x & 0x7f) | 0x80;
        putc(ch, file);
        x >>= 7;
    }
    ch = x;
    putc(ch, file);
}

void FmlaBinWriter::write_str(const char* str) {
    write_int(strlen(str));
    fputs(str, file);
}

void FmlaBinWriter::analyze(Fmla* fmla) {
    if (this->is_analyzed.count(fmla)) {return;}
    this->is_analyzed.insert(fmla);
    assert(fmla->op <= FmlaOp::MAX_OP);
    this->op_rewrite[fmla->op] = 1;
    if (fmla->op == FmlaOp::VAR) {
        this->var_count[fmla] += 1;
    } else {
        for (int i=0; i < fmla->num_args; i++) {
            analyze(fmla->arg[i]);
        }
    }
}

void FmlaBinWriter::write_hdr() {
    fprintf(file, "%s", "(FmlaBin)\n");
    write_str(this->comment);

    /* Compute renumbering of operators. */
    unsigned int num_ops = 0;
    this->op_rewrite[FmlaOp::VAR] = 0;
    for (int op=0; op <= FmlaOp::MAX_OP; op++) {
        if (this->op_rewrite[op] != 0) {
            this->op_rewrite[op] = ++num_ops;
        }
    }
    /* Write list of operators. */
    write_int(num_ops);
    for (int op=0; op <= FmlaOp::MAX_OP; op++) {
        if (this->op_rewrite[op] != 0) {
            write_str(FmlaOp::enum_to_str((FmlaOp::op)op));
        }
    }
    
    /* Write number of subformulas. */
    write_int(0);  // TODO

    /* Write list of variables. */
    this->cur_fmla_num = num_ops;
    write_int(this->var_count.size());
    foreach (it, var_count) {
        Fmla* var = it->first;
        assert(var->op == FmlaOp::VAR);
        if (!this->strip_symtab) {
            write_str((char*) var->arg[0]);
        } else {
            write_str("");
        }
        this->fmla_to_num[var] = ++cur_fmla_num;
    }
}

void FmlaBinWriter::write_rec(Fmla* fmla) {
    auto(it, fmla_to_num.find(fmla));
    if (it != fmla_to_num.end()) {
        unsigned int idx = it->second;
        assert(idx <= cur_fmla_num);
        write_int(idx);
        return;
    }
    assert(fmla->op != FmlaOp::VAR);
    write_int(op_rewrite[fmla->op]);
    write_int(fmla->num_args);
    for (int i=0; i < fmla->num_args; i++) {
        write_rec(fmla->arg[i]);
    }
    this->fmla_to_num[fmla] = ++cur_fmla_num;
}

void FmlaBinWriter::write_file(Fmla* fmla) {
    this->analyze(fmla);
    this->write_hdr();
    this->write_rec(fmla);
}


void Fmla::write_bin(FILE* out) {
    FmlaBinWriter writer = FmlaBinWriter(out);
    writer.write_file(this);
}


/************************************************************************/

FmlaBinReader::FmlaBinReader(FILE* _file) {
    this->file = _file;
    this->op_rewrite = talloc(int, FmlaOp::MAX_OP + 1);
}
FmlaBinReader::~FmlaBinReader() {
    free(this->op_rewrite);
}

int FmlaBinReader::read_int() {
    unsigned int x = 0, i = 0;
    unsigned int ch;
    while (true) {
        ch = getc(file);
        if (ch == (unsigned) EOF) {
            die_f("Unexpected EOF reading binary file!\n");
        } 
        unsigned int cur = (ch & 0x7f) << (7 * i);
        if ((cur >> (7 * i)) != (ch & 0x7f)) {
            die_f("Error decoding integer in binary file: value is too large!\n");
        }
        x |= cur;
        if ((ch & 0x80) == 0) {
            return x;
        }
        i++;
    }
}

Fmla* FmlaBinReader::read_file() {
    read_hdr();
    return read_rec();
}

char* FmlaBinReader::read_str() {
    unsigned int len = read_int();
    char* ret = talloc(char, len);
    fread(ret, sizeof(char), len, file);
    return ret;
}

void FmlaBinReader::read_hdr() {
    char newlch;
    fscanf(file, "(FmlaBin)%c", &newlch) || die_f("Bad binary file header!\n");
    int comment_len = read_int();
    assert(comment_len == 0);
    int num_ops = read_int();
    for (int op=1; op <= num_ops; op++) {
        char* op_str = read_str();
        FmlaOp::op op_new = FmlaOp::str_to_enum(op_str);
        if (op_new == FmlaOp::ERROR && !StrEqc(op_str, "var")) {
            die_f("Error: Unknown operator '%s'.\n", op_str);
        }
        this->op_rewrite[op] = op_new;
        free(op_str);
    }
    this->max_op = num_ops;
    this->cur_fmla_num = num_ops;
    this->num_decl_fmlas = read_int();
    int num_vars = read_int();
    set<Fmla*> hit_names = set<Fmla*>();
    for (int i=0; i < num_vars; i++) {
        char* var_name = read_str();
        if (strlen(var_name) == 0) {
            free(var_name);
            asprintf(&var_name, "v%i", i+1);
        }
        Fmla* var = FmlaVar(var_name);
        if (hit_names.count(var)) {
            die_f("Error: Two variables are named '%s'.\n", var_name);
        }
        hit_names.insert(var);
        num_to_fmla[++cur_fmla_num] = var;
        free(var_name);
    }
}

Fmla* FmlaBinReader::read_rec() {
    unsigned int x = read_int();
    if (x > this->max_op) {
        Fmla* ret = num_to_fmla[x];
        if (ret == NULL) {
            die_f("Error reading binary file: " 
                "Undefined formula #%i, near byte %i.\n",
                x, ftell(this->file));
        }
        return ret;
    }
    FmlaOp::op op = (FmlaOp::op) op_rewrite[x];
    unsigned int num_args = read_int();
    vector<Fmla*> args = vector<Fmla*>();
    args.reserve(num_args);
    for (unsigned int i=0; i < num_args; i++) {
        args.push_back(read_rec());
    }
    Fmla* ret = RawFmla(op, args);
    num_to_fmla[++cur_fmla_num] = ret;
    return ret;
}

/************************************************************************/

char* mk_temp_name() {
    const unsigned int SZ = 1024;
    const char *path;
    char* fullname = talloc(char, SZ);
    char const* templat = "/fmla.tmp.XXXXXX";

    path = getenv("TMPDIR");
    if (!path || *path == '\0') {path = "/tmp/";}
    if (strlen(path) + strlen(templat) >= SZ) {
        die("TMPDIR is too long!");
    }

    strncpy(fullname, path, SZ - 1);
    strncat(fullname, templat, SZ - strlen(fullname) - 1);

    int fd = mkstemp(fullname);
    if (fd < 0) {
        free(fullname);
        return NULL;
    }
    close(fd);

    return fullname;
}

/***********************************************************************/

class DimacsWriter {
    public:
    typedef vector<int> clause;
    Fmla* full_fmla;
    bool allow_quant;
    set<Fmla*>* hit;
    vector<vector<int> > quant_pfx;
    vector<clause> cnf;
    map<int, Fmla*>* lit_to_fmla;

    /* Constructor */
    DimacsWriter(Fmla* _fmla) {
        full_fmla = _fmla;
        allow_quant = true;
        hit = new set<Fmla*>();
        lit_to_fmla = NULL;
    }

    int lit_id(Fmla* fmla) {
        int ret = fmla->id;
        if (fmla->op == FmlaOp::NOT) {
            ret = -fmla->arg[0]->id;
        }
        return ret;
    }

    void process(Fmla* fmla, bool cur_allow_q) {
        if (hit->count(fmla)) {
            return;
        }
        hit->insert(fmla);
        switch(fmla->op) {
            case FmlaOp::VAR: {
                return;
            }
            case FmlaOp::NOT: {
                process(fmla->arg[0], false);
                return;
            }
            case FmlaOp::FALSE:
            case FmlaOp::TRUE:
            {
                clause cl = clause();
                int gate_id = lit_id(fmla);
                if (fmla->op == FmlaOp::TRUE) {
                    cl.push_back(gate_id);
                } else {
                    cl.push_back(-gate_id);
                }
                this->cnf.push_back(cl);
                return;
            }
            case FmlaOp::AND: {
                //  (v <==> (x1 & x2 & x3)) expands to
                //  (~x1 | ~x2 | ~x3 | v)  &  (~v | x1)  &  (~v | x2)  &  (~v | x3)
                clause cl = clause();
                int gate_id = lit_id(fmla);
                cl.push_back(gate_id);
                for (int i=0; i < fmla->num_args; i++) {
                    Fmla* cur_arg = fmla->arg[i];
                    cl.push_back(-lit_id(cur_arg));
                }
                this->cnf.push_back(cl);
                for (int i=0; i < fmla->num_args; i++) {
                    cl = clause();
                    Fmla* cur_arg = fmla->arg[i];
                    cl.push_back(-gate_id);
                    cl.push_back(lit_id(cur_arg));
                    this->cnf.push_back(cl);
                }
                for (int i=0; i < fmla->num_args; i++) {
                    process(fmla->arg[i], false);
                }
                return;
            }
            case FmlaOp::FREE: 
            case FmlaOp::EXISTS: 
            case FmlaOp::FORALL: 
            {
                if (!this->allow_quant) {
                    die_f("Error: No quantifiers allowed!\n");
                }
                if (!cur_allow_q) {
                    die_f("Error: Prenex form is required!\n");
                }
                int qnt_id = lit_id(fmla);
                int sub_id = lit_id(fmla->arg[1]);
                {
                    clause cl = clause();
                    cl.push_back(qnt_id);
                    cl.push_back(-sub_id);
                    this->cnf.push_back(cl);
                }
                {
                    clause cl = clause();
                    cl.push_back(-qnt_id);
                    cl.push_back(sub_id);
                    this->cnf.push_back(cl);
                }
                process(fmla->arg[1], true);
                return;
            }
            default:
            {
                die_f("Error: Unexpected '%s' in converting to equi-sat CNF.\n", 
                    FmlaOp::enum_to_str(fmla->op));
            }
        }
    }

    void write_quant_pfx(FILE* file) {
        bool has_quant = false;
        {
        Fmla* fmla = full_fmla;
        while (FmlaOp::is_quant(fmla->op)) {
            has_quant = true;
            char qsym;
            switch (fmla->op) {
                case FmlaOp::EXISTS: qsym = 'e'; break;
                case FmlaOp::FORALL: qsym = 'a'; break;
                case FmlaOp::FREE:   qsym = 'f'; break;
                default: abort();
            }
            fprintf(file, "%c ", qsym);
            Fmla* vars = fmla->arg[0];
            for (int i=0; i < vars->num_args; i++) {
                fprintf(file, "%i ", lit_id(vars->arg[i]));
            }
            fprintf(file, "0\n");
            fmla = fmla->arg[1];
        }
        }
        if (has_quant) {
            fprintf(file, "e ");
            foreach (it_fmla, *hit) {
                Fmla* fmla = *it_fmla;
                if (fmla->op == FmlaOp::VAR || fmla->op == FmlaOp::NOT) {
                    continue;
                }
                fprintf(file, "%i ", lit_id(fmla));
            }
            fprintf(file, "0\n");
        }
    }

    void write(FILE* file) {
        clause top_cl = clause();
        int top_id = lit_id(full_fmla);
        top_cl.push_back(top_id);
        cnf.push_back(top_cl);
        process(full_fmla, true);
        {
            set<Fmla*>* vars = new set<Fmla*>();
            full_fmla->find_vars_in_fmla(vars);
            foreach (itVar, *vars) {
                Fmla* var = *itVar;
                fprintf(file, "c VarName %i : %s\n", var->id, (char*) var->arg[0]);
                if (lit_to_fmla) {
                    (*lit_to_fmla)[var->id] = var;
                    (*lit_to_fmla)[-var->id] = var->negate();
                }
            }
        }
        fprintf(file, "p cnf %i %i\n", next_fmla_num, (int) cnf.size());
        write_quant_pfx(file);
        foreach (it_cl, cnf) {
            foreach (it_lit, *it_cl) {
                fprintf(file, "%i ", *it_lit);
            }
            fprintf(file, "0\n");
        }
    };
};


Fmla* Fmla::get_sat_asgn(const char* minisat_exe, struct minisat_stats* p_stats) {
    DimacsWriter dimacs_writer = DimacsWriter(this->to_nnf()->nnf_to_aig());
    dimacs_writer.allow_quant = false;
    map<int, Fmla*> lit_to_fmla = map<int, Fmla*>();
    dimacs_writer.lit_to_fmla = &lit_to_fmla;
    char* dimacs_filename = mk_temp_name();
    FILE* dimacs_file = fopen(dimacs_filename, "wb");
    char* out_filename = mk_temp_name();
    dimacs_writer.write(dimacs_file);
    fflush(dimacs_file);
    char* cmd = NULL;
    asprintf(&cmd, "%s %s %s >/dev/null 2>/dev/null", minisat_exe, dimacs_filename, out_filename);
    long long t_start_user = GetUserTimeMicro();
    long long t_start_clock = GetClockTimeMicro();
    system(cmd);
    if (p_stats != NULL) {
        p_stats->user_time =  ((double)GetUserTimeMicro() - t_start_user) / 1e6;
        p_stats->clock_time = ((double)GetClockTimeMicro() - t_start_clock) / 1e6;
    }

    // asprintf(&cmd, "cat %s %s", out_filename, dimacs_filename);
    // system(cmd); free(cmd);

    FILE* out_file = fopen(out_filename, "rb");
    vector<Fmla*> ret_lits;
    Fmla* ret_fmla = NULL;
    {
        char* buf = NULL;
        fscanf(out_file, "%as", &buf);
        if (StrEqc(buf, "UNSAT")) {
            ret_fmla = fmla_false;
            goto exit_sat_asgn;
        }
        if (!StrEqc(buf, "SAT")) {
            ret_fmla = fmla_error;
            goto exit_sat_asgn;
        }
        while (true) {
            int lit_num;
            fscanf(out_file, "%i", &lit_num);
            if (lit_num == 0) {
                ret_fmla = RawFmla(FmlaOp::LIST, ret_lits);
                goto exit_sat_asgn;
            }
            auto(it_fmla, lit_to_fmla.find(lit_num));
            if (it_fmla != lit_to_fmla.end()) {
                Fmla* fmla_lit = it_fmla->second;
                assert(fmla_lit != NULL);
                ret_lits.push_back(fmla_lit);
            }
        }
    }
    exit_sat_asgn:
    free(cmd);
    fclose(dimacs_file);
    fclose(out_file);
    remove(dimacs_filename);  free(dimacs_filename);
    remove(out_filename);     free(out_filename);
    if (ret_fmla == fmla_error) {
        fprintf(stderr, "Error executing MiniSat or reading its output.\n");
    }
    return ret_fmla;
}

void Fmla::write_gq(FILE* out, std::set<Fmla*>* hit) {
    if (hit == NULL) {
        hit = new set<Fmla*>();
        set<Fmla*>* vars = new set<Fmla*>();
        int max_var_num = 0;
        this->find_vars_in_fmla(vars);
        foreach (itVar, *vars) {
            max_var_num = max(max_var_num, (*itVar)->id);
        }
        fprintf(out, "CktQBF\n");
        fprintf(out, "LastInputVar %i\n", max_var_num);
        fprintf(out, "LastGateVar %i\n", next_fmla_num);
        fprintf(out, "OutputGateLit %i\n", this->id);
        fprintf(out, "\n");
        foreach (itVar, *vars) {
            Fmla* var = *itVar;
            fprintf(out, "VarName %i : %s\n", var->id, (char*) var->arg[0]);
        }
        fprintf(out, "\n");
        this->write_gq(out, hit);
        delete hit;
        return;
    }
    if (hit->count(this)) {
        return;
    }
    hit->insert(this);
    if (this->op == FmlaOp::VAR) {
        return;
    }
    if (this->op == FmlaOp::NOT) {
        this->arg[0]->write_gq(out, hit);
        return;
    }
    for (int i=0; i < this->num_args; i++) {
        if (this->arg[i]->op != FmlaOp::LIST) {continue;}
        this->arg[i]->write_gq(out, hit);
    }
    fprintf(out, "%i = %s(", this->id, FmlaOp::enum_to_str(this->op));
    for (int i=0; i < this->num_args; i++) {
        Fmla* cur_arg = this->arg[i];
        int cur_id = cur_arg->id;
        if (cur_arg->op == FmlaOp::NOT) {
            cur_id = -cur_arg->arg[0]->id;
        }
        fprintf(out, "%i", cur_id);
        if (i < this->num_args - 1) {
            fprintf(out, ", ");
        }
    }
    fprintf(out, ")\n");
    for (int i=0; i < this->num_args; i++) {
        if (this->arg[i]->op == FmlaOp::LIST) {continue;}
        this->arg[i]->write_gq(out, hit);
    }
}

int last_9_digits_to_int(const char* s) {
    int start = 0;
    int len = strlen(s);
    if (len > 9) {
        start = len - 9;
    }
    return atoi(s + start);
}

bool is_number(const char* s) {
    if (*s == '\0') {return false;}
    for (; *s != '\0'; s++) {
        if (!(('0' <= *s) && (*s <= '9'))) {
            return false;
        }
    }
    return true;
}

void replace_char(char* s, char old_c, char new_c) {
    for (; *s != '\0'; s++) {
        if (*s == old_c) {
            *s = new_c;
        }
    }
}

char* Fmla::get_tmp_pfx(int* next_val) {
    const char* orig_pfx = "tmp";
    int orig_pfx_len = strlen(orig_pfx);
    set<Fmla*>* var_set = new set<Fmla*>();
    this->find_vars_in_fmla(var_set);
    int max_len = 0;
    int max_val = 0;
    foreach(it_var, *var_set) {
        const char* var_name = (*it_var)->name_of_var();
        if (strncmp(orig_pfx, var_name, orig_pfx_len) != 0) {
            continue;
        }
        var_name += orig_pfx_len;
        if (!is_number(var_name)) {
            continue;
        }
        int name_len = strlen(var_name);
        if (max_len < name_len) {
            max_len = name_len;
            max_val = 0;
        }
        max_val = max(max_val, last_9_digits_to_int(var_name));
    }
    if (max_val > 500000000) {
        max_val = 0;
        max_len++;
    }
    *next_val = max_val + 1;
    int pad9 = max(0, max_len - 9);
    char* ret = NULL;
    asprintf(&ret, "%s%*s", orig_pfx, pad9, "");
    replace_char(ret, ' ', '9');
    return ret;
}

void Fmla::number_subfmlas(std::map<Fmla*,int>& fmla_to_num, int* cur_num) {
    if (this->op == FmlaOp::VAR) {return;}
    int& cache_entry = fmla_to_num[this];
    if (cache_entry != 0) {
        return;
    }
    for (int i=0; i < this->num_args; i++) {
        this->arg[i]->number_subfmlas(fmla_to_num, cur_num);
    }
    cache_entry = ++*cur_num;
}

void Fmla::write_qcir(FILE* out) {
    std::set<Fmla*>* hit = new set<Fmla*>();
    std::map<Fmla*,int> fmla_to_num = std::map<Fmla*,int>();
    int start_gate_num;
    char* gate_pfx = this->get_tmp_pfx(&start_gate_num);
    this->number_subfmlas(fmla_to_num, &start_gate_num);
    fprintf(out, "#QCIR-G14\n");
    fprintf(out, "output(");
    this->write_qcir_lit(out, fmla_to_num, gate_pfx);
    fprintf(out, ")\n");
    this->write_qcir_rec(out, hit, fmla_to_num, gate_pfx);
    delete hit;
}

void Fmla::write_qcir_lit(FILE* out, std::map<Fmla*,int>& fmla_to_num, const char* pfx) {
    if (this->op == FmlaOp::NOT) {
        fprintf(out, "-");
        this->arg[0]->write_qcir_lit(out, fmla_to_num, pfx);
        return;
    }
    if (this->op == FmlaOp::VAR) {
        fprintf(out, "%s", this->name_of_var());
    } else {
        int gate_num = fmla_to_num[this];
        assert(gate_num != 0);
        if (pfx[strlen(pfx) - 1] != '9') {
            fprintf(out, "%s%i", pfx, gate_num);
        } else {
            fprintf(out, "%s%09i", pfx, gate_num);
        }
    }
}

void Fmla::write_qcir_rec(FILE* out, std::set<Fmla*>* hit, std::map<Fmla*,int>& fmla_to_num, const char* pfx) {
    if (hit->count(this)) {
        return;
    }
    hit->insert(this);
    FmlaOp::op this_op = this->op;
    switch (this_op) {
        case FmlaOp::VAR:
            return;
        case FmlaOp::TRUE:
            this_op = FmlaOp::AND; break;
        case FmlaOp::FALSE:
            this_op = FmlaOp::OR; break;
        default: ;
    }
    if (this_op == FmlaOp::NOT) {
        this->arg[0]->write_qcir_rec(out, hit, fmla_to_num, pfx);
        return;
    }
    if (this->is_quant()) {
        this->arg[1]->write_qcir_rec(out, hit, fmla_to_num, pfx);
    } else {
        for (int i=0; i < this->num_args; i++) {
            this->arg[i]->write_qcir_rec(out, hit, fmla_to_num, pfx);
        }
    }
    this->write_qcir_lit(out, fmla_to_num, pfx);
    fprintf(out, " = %s(", FmlaOp::enum_to_str(this_op));
    Fmla* fmla = (this->is_quant() ? this->arg[0] : this);
    for (int i=0; i < fmla->num_args; i++) {
        fmla->arg[i]->write_qcir_lit(out, fmla_to_num, pfx);
        if (i < fmla->num_args - 1) {
            fprintf(out, ", ");
        }
    }
    if (this->is_quant()) {
        fprintf(out, "; ");
        this->arg[1]->write_qcir_lit(out, fmla_to_num, pfx);
    }
    fprintf(out, ")\n");
}

std::map<Fmla*,Fmla*>* Fmla::assoc_list_to_map() {
    map<Fmla*,Fmla*>* ret = new map<Fmla*,Fmla*>();
    map<Fmla*,Fmla*>& retmap = *ret;

    if (this->op != FmlaOp::LIST) {
        printf_err("Error: Expected a list, but found a %s.\n", FmlaOp::enum_to_str(this->op));
        delete ret;
        return NULL;
    }
        
    for (int i=0; i < this->num_args; i++) {
        Fmla* cur_arg = this->arg[i];
        if (cur_arg->op != FmlaOp::LIST) {
            printf_err("Error: Expected a two-item list, but found a %s.\n", 
                FmlaOp::enum_to_str(this->op));
            delete ret;
            return NULL;
        }
        if (cur_arg->num_args != 2) {
            printf_err("Error: Expected two items in sublist, but found %i.\n", 
                cur_arg->num_args);
            delete ret;
            return NULL;
        }
        retmap[cur_arg->arg[0]] = cur_arg->arg[1];
    }

    return ret;
}

extern "C" void init_fmla() {
    auto(EmptyVec, vector<Fmla*>(0, NULL));
    if (fmla_true != NULL) {die("Error: init_fmla called more than once!\n");}
    fmla_true  = RawFmla<FmlaPtrVector>(FmlaOp::TRUE,  EmptyVec);
    fmla_false = RawFmla<FmlaPtrVector>(FmlaOp::FALSE, EmptyVec);
    fmla_error = RawFmla<FmlaPtrVector>(FmlaOp::ERROR, EmptyVec);
}

SeqCkt::SeqCkt(Fmla* fmla) {
    for (int i=0; i < fmla->num_args; i++) {
        Fmla* arg = fmla->arg[i];
        if (arg->op != FmlaOp::LIST || arg->num_args != 2) {
            die("Bad circuit!\n");
        }
        const char* sec_name = arg->arg[0]->name_of_var();
        Fmla* sec_data = arg->arg[1];
        if (sec_data->op != FmlaOp::LIST) {
            die_f("Bad circuit: section %s.\n", sec_name);
        }
        if (0) {
        } else if (StrEqc(sec_name, "DEFINE")) {
            /* This section is only needed for subformula definitions. */
        } else if (StrEqc(sec_name, "INPUTS")) {
            this->inputs = sec_data;
        } else if (StrEqc(sec_name, "LATCHES")) {
            this->latches = sec_data;
        } else if (StrEqc(sec_name, "OUTPUTS")) {
            this->outputs = sec_data;
        } else if (StrEqc(sec_name, "SPEC_AG")) {
        } else {
            die_f("Unknown section '%s'.\n", sec_name);
        }
    }
}

Fmla* SeqCkt::get_trans_rel() {
    Fmla* ret;
    vector<Fmla*> trans_conds = vector<Fmla*>();
    for (int i=0; i < latches->num_args; i++) {
        assert(latches->arg[i]->num_args == 2);
        trans_conds.push_back(RawFmla(FmlaOp::EQ, 
            latches->arg[i]->arg[0],
            latches->arg[i]->arg[1]));
    }
    ret = RawFmla(FmlaOp::AND, trans_conds);
    return ret;
}

Fmla* SeqCkt::new_latch_to_old(Fmla* new_latch) {
    const char* new_name = new_latch->name_of_var();
    const char* pfx = "next<";
    assert(strstr(new_name, pfx) == new_name);
    char* old_name = NULL;
    asprintf(&old_name, "old<%s", new_name + strlen(pfx));
    Fmla* old_latch = FmlaVar(old_name);
    free(old_name);
    return old_latch;
}

Fmla* SeqCkt::reach_fwd_one() {
    Fmla* trans_rel = this->get_trans_rel();
    vector<Fmla*> init_conds = vector<Fmla*>();
    vector<Fmla*> free_vars = vector<Fmla*>();
    vector<Fmla*> elim_vars = vector<Fmla*>();
    for (int i=0; i < inputs->num_args; i++) {
        elim_vars.push_back(inputs->arg[i]);
    }
    for (int i=0; i < latches->num_args; i++) {
        assert(latches->arg[i]->num_args == 2);
        Fmla* cur_latch = latches->arg[i]->arg[0];
        Fmla* old_latch = new_latch_to_old(cur_latch);
        free_vars.push_back(cur_latch);
        elim_vars.push_back(old_latch);
        init_conds.push_back(RawFmla(FmlaOp::EQ, old_latch, fmla_false));
    }
    return RawFmla(FmlaOp::FREE, RawFmla(FmlaOp::LIST, free_vars),
            RawFmla(FmlaOp::EXISTS, RawFmla(FmlaOp::LIST, elim_vars),
            ConsFmla(FmlaOp::AND, RawFmla(FmlaOp::AND, init_conds), trans_rel)));
}

Fmla* SeqCkt::reach_back_one() {
    Fmla* trans_rel = this->get_trans_rel();
    vector<Fmla*> final_conds = vector<Fmla*>();
    vector<Fmla*> free_vars = vector<Fmla*>();
    vector<Fmla*> elim_vars = vector<Fmla*>();
    for (int i=0; i < inputs->num_args; i++) {
        elim_vars.push_back(inputs->arg[i]);
    }
    map<Fmla*,Fmla*> map_old_to_new = map<Fmla*,Fmla*>();
    for (int i=0; i < latches->num_args; i++) {
        assert(latches->arg[i]->num_args == 2);
        Fmla* cur_latch = latches->arg[i]->arg[0];
        Fmla* old_latch = new_latch_to_old(cur_latch);
        free_vars.push_back(old_latch);
        elim_vars.push_back(cur_latch);
        map_old_to_new[old_latch] = cur_latch;
    }
    assert(this->outputs->num_args == 1);
    assert(this->outputs->arg[0]->num_args == 2);
    Fmla* bad = this->outputs->arg[0]->arg[1];
    map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
    bad = bad->subst(map_old_to_new, cache);
    final_conds.push_back(bad);
    return RawFmla(FmlaOp::FREE, RawFmla(FmlaOp::LIST, free_vars),
            RawFmla(FmlaOp::EXISTS, RawFmla(FmlaOp::LIST, elim_vars),
            ConsFmla(FmlaOp::AND, RawFmla(FmlaOp::AND, final_conds), trans_rel)));
}


Fmla* SeqCkt::to_fmla() {
    vector<Fmla*> secs = vector<Fmla*>();
    secs.push_back(RawFmla(FmlaOp::LIST, FmlaVar("INPUTS"),  this->inputs));
    secs.push_back(RawFmla(FmlaOp::LIST, FmlaVar("LATCHES"), this->latches));
    secs.push_back(RawFmla(FmlaOp::LIST, FmlaVar("OUTPUTS"), this->outputs));
    return RawFmla(FmlaOp::LIST, secs);
}

#ifdef FMLA_MAIN

int main(int argc, char **argv) {
    if (argc < 2 || (StrEqc(argv[1], "-h") || StrEqc(argv[1], "--help"))) {
        fprintf(stderr, "Usage: %s infile [OPTIONS]\n", argv[0]);
        fprintf(stderr, "       %s \"-e <EXPR>\" [OPTIONS]\n", argv[0]);
        fprintf(stderr, 
            "Options:\n"
            " '-read-qcir':     Read input formula in QCIR format.\n"
            " '-qcir-ren-tab':  Use renaming table generated by qcir-conv.py.\n"
            " '-write-gq':      Write input formula in GhostQ format.\n"
            " '-write-qcir':    Write input formula in QCIR format.\n"
            " '-write-dimacs':  Write input formula in DIMACS format.\n"
            " '-max-ind <n>':   Maximum indent level (in spaces).\n"
            " '-max-col <n>':   Maximum line length (in bytes).\n"
            " '-write-bin':     Write input formula in binary format.\n"
            " '-strip-syms':    Strip symbol table in binary format.\n"
            " '-o <file>':      Write output to designated file.\n"
            " '-check-sat </path/to/minisat>': Returns a satisfying assignment or 'false()'.\n"
            " '-size':          Print the DAG size of the formula.\n"
            " '-tree-size':     Print the tree size of the formula.\n"
            " '-bdd':           Convert input formula to a BDD.\n"
            " '-bdd-simp':      Convert input formula to a BDD and simplify.\n"
            " '-bdd-order <file>': File specifying the order of variables.  It should\n"
            "                   be a list like '[var1, var2, var3]'.\n"
            " '-no-flatten':    Don't flatten AND/OR gates.\n"
            " '-find-ite':      Attempt to find ITEs in an AIG.\n"
            " '-to-nnf':        Convert input formula to NNF.\n"
            " '-to-aig':        Convert input formula to AIG (with multiple fan-in).\n"
            " '-no-eval-resolve':  Don't evaluate RESOLVE operations.\n"
            " '-only-last':     If the input is a list, only print the last item.\n"
            " '-seq-ckt':       Treats inputs as a sequential circuit from AIGER.\n"
            " '-reach-fwd-one': Formula for states forward-reachable in one step.\n"
            " '-reach-back-one': Formula for states backward-reachable in one step.\n"
            " '-rename <file>': Same as '-subst'.\n"
            " '-subst <file>':  Substitution; use '-help-subst' for more info.\n"
            //" '-lprev <p> <s>': Prefix and suffix for old value of latch.\n"
            //" '-lnext <p> <s>': Prefix and suffix for new value of latch.\n"
            " See source code for additional options.\n"
        );
        exit(1);
    }
    if (StrEqc(argv[1], "-help-subst")) {
        fprintf(stderr, "%s", 
        "Help for '-subst FILE': FILE should be an association list that maps variables\n"
        "to formulas.  For example, if FILE is the formula '[[x, and(y,z)], [y, w]]',\n"
        "then x will be substituted with and(y,z), and y will be substituted with w.\n");
        exit(1);
    }
    char* filename = argv[1];
    if (strlen(filename) > 1 && filename[0] == '-' && strncmp(filename, "-e ", 3) != 0) {
        die_f("First argument must be a filename, but it begins with a dash ('%s').\n", filename);
    }

    bool read_qcir = false;
    bool write_gq = false;
    bool write_qcir = false;
    bool write_dimacs = false;
    bool write_bin = false;
    bool strip_syms = false;
    bool check_sat = false;
    bool write_size = false;
    bool write_tree_size = false;
    bool write_nnf = false;
    bool write_aig = false;
    bool write_fmla = true;
    bool eval_resolve = true;
    bool no_flatten = false;
    bool find_ite = false;
    int max_indent = 40;
    int max_col = 79;
    bool write_bdd = false;
    bool pretty_bdd = false;
    bool only_last = false;
    bool seq_ckt = false;
    bool reach_fwd_one = false;
    bool reach_back_one = false;
    const char* rename_filename = NULL;
    const char* output_filename = NULL;
    const char* minisat_exe = "minisat";
    const char* bdd_order_file = NULL;
    // char* lprev_prefix = "";
    // char* lprev_suffix = "";
    // char* lnext_prefix = "";
    // char* lnext_suffix = "";
    for (int ixArg=2; ixArg < argc; ixArg++) {
        char* sArg = argv[ixArg];
        if (sArg[0] == '-' && sArg[1] == '-') {
            sArg += 1;
        }
        if (false) {}
        else if (StrEqc(sArg, "-no-write-fmla")) {write_fmla = false;}
        else if (StrEqc(sArg, "-read-qcir")) {read_qcir = true;}
        else if (StrEqc(sArg, "-qcir-ren-tab")) {glo_qcir_ren = true;}
        else if (StrEqc(sArg, "-write-gq")) {write_gq = true; write_fmla = false;}
        else if (StrEqc(sArg, "-write-qcir")) {write_qcir = true; write_fmla = false;}
        else if (StrEqc(sArg, "-write-dimacs")) {write_dimacs = true; write_fmla = false;}
        else if (StrEqc(sArg, "-write-bin")) {write_bin = true; write_fmla = false;}
        else if (StrEqc(sArg, "-strip-syms")) {strip_syms = true;}
        else if (StrEqc(sArg, "-size")) {write_size = true; write_fmla = false;}
        else if (StrEqc(sArg, "-tree-size")) {write_tree_size = true; write_fmla = false;}
        else if (StrEqc(sArg, "-to-nnf")) {write_nnf = true;}
        else if (StrEqc(sArg, "-to-aig")) {write_aig = true;}
        else if (StrEqc(sArg, "-no-flatten")) {no_flatten = true;}
        else if (StrEqc(sArg, "-find-ite")) {find_ite = true;}
        else if (StrEqc(sArg, "-no-eval-resolve")) {eval_resolve = false;}
        else if (StrEqc(sArg, "-bdd")) {write_bdd = true;}
        else if (StrEqc(sArg, "-bdd-simp")) {write_bdd = true; pretty_bdd = true;}
        else if (StrEqc(sArg, "-only-last")) {only_last = true;}
        else if (StrEqc(sArg, "-seq-ckt")) {seq_ckt = true;}
        else if (StrEqc(sArg, "-reach-one")) {reach_fwd_one = true;}
        else if (StrEqc(sArg, "-reach-fwd-one")) {reach_fwd_one = true;}
        else if (StrEqc(sArg, "-reach-back-one")) {reach_back_one = true;}
        else if (StrEqc(sArg, "-check-sat")) {
            check_sat = true; 
            write_fmla = false;
            if (ixArg + 1 >= argc) {die_f("Option '%s' requires a parameter.\n", sArg);}
            minisat_exe = argv[++ixArg];
        } else if (StrEqc(sArg, "-subst") || StrEqc(sArg, "-rename")) {
            if (ixArg + 1 >= argc) {die_f("Option '%s' requires a parameter.\n", sArg);}
            rename_filename = argv[++ixArg];
        } else if (StrEqc(sArg, "-o")) {
            if (ixArg + 1 >= argc) {die_f("Option '%s' requires a parameter.\n", sArg);}
            output_filename = argv[++ixArg];
        } else if (StrEqc(sArg, "-max-col")) {
            if (ixArg + 1 >= argc) {die_f("Option '%s' requires a parameter.\n", sArg);}
            max_col = atoi(argv[++ixArg]);
        } else if (StrEqc(sArg, "-max-ind")) {
            if (ixArg + 1 >= argc) {die_f("Option '%s' requires a parameter.\n", sArg);}
            max_indent = atoi(argv[++ixArg]);
        } else if (StrEqc(sArg, "-bdd-order")) {
            if (ixArg + 1 >= argc) {die_f("Option '%s' requires a parameter.\n", sArg);}
            bdd_order_file = argv[++ixArg];
        } else {
            fprintf(stderr, "Invalid command-line option: '%s'.\n", sArg);
            exit(1);
        }
    }

    init_fmla();
    Fmla* fmla = NULL;
    if (read_qcir) {
        fmla = parse_qcir_file(filename);
    } else {
        fmla = parse_fmla_file(filename);
    }
    if (fmla == NULL) {
        die("No formula!\n");
    }
    if (only_last) {
        if (fmla->op == FmlaOp::LIST) {
            fmla = fmla->arg[fmla->num_args - 1];
        } else if (fmla->op == FmlaOp::FREE && fmla->arg[1]->op == FmlaOp::LIST) {
            Fmla* sublist = fmla->arg[1];
            fmla = ConsFmla(FmlaOp::FREE, fmla->arg[0], sublist->arg[sublist->num_args - 1]);
        }
    }
    Fmla* bdd_order = NULL;
    if (bdd_order_file) {
        bdd_order = parse_fmla_file(bdd_order_file);
        if (bdd_order->op != FmlaOp::LIST) {
            die("Error: contents of BDD Order file is not a list!\n");
        }
    }
    if (eval_resolve) {
        map<Fmla*,Fmla*> resolve_cache = map<Fmla*,Fmla*>();
        fmla = fmla->eval_resolve(resolve_cache);
    }
    if (find_ite && !write_aig) {
        fmla = fmla->find_ites_in_aig();
    }
    if (seq_ckt) {
        fmla = SeqCkt(fmla).to_fmla();
    }
    if (reach_fwd_one) {
        SeqCkt ckt = SeqCkt(fmla);
        fmla = ckt.reach_fwd_one();
    }
    if (reach_back_one) {
        SeqCkt ckt = SeqCkt(fmla);
        fmla = ckt.reach_back_one();
    }
    if (!no_flatten) {
        fmla = fmla->flatten_andor();
    }
    if (write_bdd) {
        fmla = fmla->to_bdd(bdd_order);
        if (pretty_bdd) {
            fmla = fmla->simp_ite();
        }
    }
    if (write_nnf) {
        fmla = fmla->to_nnf();
    }
    if (write_aig) {
        fmla = fmla->to_nnf()->nnf_to_aig();
    }
    if (find_ite && write_aig) {
        fmla = fmla->find_ites_in_aig();
    }
    if (!no_flatten) {
        fmla = fmla->flatten_andor();
    }
    if (rename_filename) {
        Fmla* rename_map_fmla = parse_fmla_file(rename_filename);
        map<Fmla*,Fmla*> cache = map<Fmla*,Fmla*>();
        map<Fmla*,Fmla*>* rename_map = rename_map_fmla->assoc_list_to_map();
        fmla = fmla->subst(*rename_map, cache);
    }
    FILE* outfile = stdout;
    if (output_filename) {
        outfile = fopen(output_filename, "wb");
    }
    if (write_fmla) {
        FmlaWriterOpts opts = FmlaWriterOpts();
        opts.max_col = max_col;
        opts.max_indent = max_indent;
        opts.out = outfile;
        fmla->write(opts);
        fprintf(outfile, "\n");
    }
    if (write_gq) {
        Fmla* simp_fmla = fmla->to_nnf();
        if (write_aig) {
            simp_fmla = simp_fmla->nnf_to_aig();
        }
        simp_fmla->write_gq(outfile, NULL);
    }
    if (write_qcir) {
        fmla->write_qcir(outfile);
    }
    if (write_size) {printf("%2i\n", fmla->dag_size());}
    if (write_tree_size) {printf("%g\n", fmla->tree_size());}
    if (write_dimacs) {
        DimacsWriter(fmla->to_nnf()->nnf_to_aig()).write(outfile);
    }
    if (write_bin) {
        if (isatty(fileno(outfile))) {
            printf_err("Won't write binary output to a tty.\n");
            exit(1);
        }
        FmlaBinWriter writer = FmlaBinWriter(outfile);
        writer.strip_symtab = strip_syms;
        writer.write_file(fmla);
    }
    if (check_sat) {
        Fmla* sat_asgn = fmla->get_sat_asgn(minisat_exe, NULL);
        sat_asgn->write(outfile);
        fprintf(outfile, "\n");
    }

    // #define num_vars 300000
    // Fmla** int_to_var = (Fmla**) malloc(num_vars * sizeof(Fmla*));
    // for (int i=0; i < num_vars; i++) {
    //     char* temp = NULL;
    //     asprintf(&temp, "%i", i);
    //     int_to_var[i] = FmlaVar(temp);
    //     //printf("%i ", int_to_var[i]->hash);
    //     //SymTab[temp];
    // }

}

#endif




