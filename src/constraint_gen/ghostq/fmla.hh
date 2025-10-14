#include <vector>
#include <map>


namespace FmlaOp {
    enum op {
        ERROR = 0,
        VAR,
        TRUE,
        FALSE,

        NOT,
        AND,
        OR,
        ITE,
        EQ,
        IMPL,
        XOR,

        LIST,
        EXISTS,
        FORALL,
        FREE,
        GSEQ,

        SUBST,
        RESOLVE,

        NEWENV,
        INCLUDE,

        MAX_OP
    };
    const char* enum_to_str(op op);
    op str_to_enum(const char* OpStr);
    #ifdef FMLA_CPP
    inline int is_quant(op op) {
        switch (op) {
            case EXISTS: 
            case FORALL: 
            case FREE: 
                return true;
            default:
                return false;
        }
    }
    inline int StrEqc(const char* s1, const char* s2) {
        if ((s1 == NULL) ^ (s2 == NULL)) {return 0;}
        return (strcasecmp(s1, s2) == 0);
    }
    const char* enum_to_str(op op) {
        switch(op) {
            case ERROR:     return "error";
            case VAR:       return "var";
            case TRUE:      return "true";
            case FALSE:     return "false";
            case NOT:       return "not";
            case AND:       return "and";
            case OR:        return "or";
            case ITE:       return "ite";       // 'If-Then-Else'
            case EQ:        return "eq";        // '<=>'
            case IMPL:      return "impl";
            case XOR:       return "xor";
            case LIST:      return "list";
            case EXISTS:    return "exists";
            case FORALL:    return "forall";
            case FREE:      return "free";
            case GSEQ:      return "gseq";
            case SUBST:     return "subst";
            case RESOLVE:   return "resolve";
            case NEWENV:    return "newenv";
            case INCLUDE:   return "include";
            default:        return "unknown";
        }
    }
    op str_to_enum(const char* OpStr) {
        op OpEnum;
        if (0) {}
        else if (StrEqc(OpStr, "true"))   {OpEnum = FmlaOp::TRUE;}
        else if (StrEqc(OpStr, "false"))  {OpEnum = FmlaOp::FALSE;}
        else if (StrEqc(OpStr, "not"))    {OpEnum = FmlaOp::NOT;}
        else if (StrEqc(OpStr, "and"))    {OpEnum = FmlaOp::AND;}
        else if (StrEqc(OpStr, "or"))     {OpEnum = FmlaOp::OR;}
        else if (StrEqc(OpStr, "ite"))    {OpEnum = FmlaOp::ITE;}
        else if (StrEqc(OpStr, "eq"))     {OpEnum = FmlaOp::EQ;}
        else if (StrEqc(OpStr, "impl"))   {OpEnum = FmlaOp::IMPL;}
        else if (StrEqc(OpStr, "xor"))    {OpEnum = FmlaOp::XOR;}
        else if (StrEqc(OpStr, "list"))   {OpEnum = FmlaOp::LIST;}
        else if (StrEqc(OpStr, "exists")) {OpEnum = FmlaOp::EXISTS;}
        else if (StrEqc(OpStr, "forall")) {OpEnum = FmlaOp::FORALL;}
        else if (StrEqc(OpStr, "free"))   {OpEnum = FmlaOp::FREE;}
        else if (StrEqc(OpStr, "gseq"))   {OpEnum = FmlaOp::GSEQ;}
        else if (StrEqc(OpStr, "subst"))  {OpEnum = FmlaOp::SUBST;}
        else if (StrEqc(OpStr, "resolve")){OpEnum = FmlaOp::RESOLVE;}
        else if (StrEqc(OpStr, "newenv")) {OpEnum = FmlaOp::NEWENV;}
        else if (StrEqc(OpStr, "include")){OpEnum = FmlaOp::INCLUDE;}
        else {OpEnum = FmlaOp::ERROR;}
        return OpEnum;
    }
    #endif
};

struct FmlaWriterOpts;

struct Fmla {
    FmlaOp::op op;
    int id;
    int num_args;
    int hash;
    Fmla *arg[0];  /* If op==VAR, then arg[0] is actually a string (i.e., char*). */

    bool operator==(Fmla& other) const;
    bool operator<(Fmla& other) const;
    Fmla operator=(Fmla& src);

    #define FPROTO_Fmla
    #include "fmla.proto"
    #undef FPROTO_Fmla
    
    static Fmla* parse_str(const char* str);
};

struct minisat_stats {
    double clock_time;
    double user_time;
};

// namespace FmlaCNF {
//     struct clause {
//         int n_lits;
//         int lit[0];
//     }
// }

struct SeqCkt {
    Fmla* inputs;
    Fmla* latches;
    Fmla* outputs;
        
    /* Constructors */
    SeqCkt(){};
    SeqCkt(Fmla*);

    #define FPROTO_SeqCkt
    #include "fmla.proto"
    #undef FPROTO_SeqCkt
};

struct FmlaBinWriter {
    FILE* file;
    const char* comment;
    bool strip_symtab;
    int* op_rewrite;
    unsigned int cur_fmla_num;
    std::set<Fmla*> is_analyzed;
    std::map<Fmla*, int> var_count;
    std::map<Fmla*, unsigned int> fmla_to_num;

    FmlaBinWriter(FILE*);
    ~FmlaBinWriter();
    #define FPROTO_FmlaBinWriter
    #include "fmla.proto"
    #undef FPROTO_FmlaBinWriter
};

struct FmlaBinReader {
    FILE* file;
    const char* comment;
    int* op_rewrite;
    unsigned int max_op;
    unsigned int cur_fmla_num;
    unsigned int num_decl_fmlas;
    std::map<unsigned int, Fmla*> num_to_fmla;

    FmlaBinReader(FILE*);
    ~FmlaBinReader();
    #define FPROTO_FmlaBinReader
    #include "fmla.proto"
    #undef FPROTO_FmlaBinReader
};


extern Fmla* fmla_true;
extern Fmla* fmla_false;
extern Fmla* fmla_error;
extern "C" void init_fmla();
extern "C" Fmla* FmlaVar(const char* var);
extern Fmla* RawFmla(const char* OpStr, std::vector<Fmla*> argvec);
extern Fmla* RawFmla(FmlaOp::op op, std::vector<Fmla*> argvec);
extern Fmla* RawFmla(FmlaOp::op op, Fmla*);
extern Fmla* RawFmla(FmlaOp::op op, Fmla*, Fmla*);
extern Fmla* RawFmla(FmlaOp::op op, Fmla*, Fmla*, Fmla*);
extern Fmla* ConsFmla(FmlaOp::op op, std::vector<Fmla*> argvec);
extern Fmla* ConsFmla(FmlaOp::op op, Fmla* x1);
extern Fmla* ConsFmla(FmlaOp::op op, Fmla* x1, Fmla* x2);
extern Fmla* ConsFmla(FmlaOp::op op, Fmla* x1, Fmla* x2, Fmla* x3);
extern "C" Fmla* ConsFmlaArr(FmlaOp::op op, int num_args, Fmla** args);
extern "C" void write_fmla(Fmla* fmla, FILE* out);
extern void dump_fmla(Fmla* fmla) __attribute__((noinline));
extern Fmla* parse_fmla_file(const char* filename);

namespace FmlaParserNS {

    using namespace std;

    typedef pair<char, vector<int> > PrefixBlock;
    typedef pair<const char*, vector<int> > GateDef;

    /* String Array Pointer */
    struct sap {
        char* p;    // pointer
        int n;      // size
    };

    class FmlaLexer {
        public:
        const char* filename;
        FILE* infile;
        FILE* tmp_file;
        sap CurLine;
        sap CurTok;
        sap SavedTok;
        int CurPos;
        int LineNum;
	bool SaveComments;

        #define FPROTO_FmlaLexer
        #include "fmla.proto"
        #undef FPROTO_FmlaLexer

        static FmlaLexer* new_from_file(const char* _filename);

        /* Constructors */
        FmlaLexer();
        /* Destructor */
        ~FmlaLexer();

    };

}
