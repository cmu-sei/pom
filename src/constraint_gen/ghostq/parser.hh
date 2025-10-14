#include <vector>
#include <map>

namespace GhostQ_Parser {

    using namespace std;

    class Lexer;

    typedef pair<char, vector<int> > PrefixBlock;
    typedef pair<const char*, vector<int> > GateDef;

    class Parser {
        public:
        int LastInputVar;
        int LastGateVar;
        int OutputGateLit;
        int PreprocTimeMilli;
        map<int, const char*> VarNames;
        map<int, vector<PrefixBlock> > GateQuants;
        map<int, GateDef> GateDefs;

        const char* OP_AND;
        const char* OP_OR;
        const char* OP_FORALL;
        const char* OP_EXISTS;
        const char* OP_FREE;
        const char* OP_LIST;

        Parser() {
            OP_AND    = "and";
            OP_OR     = "or";
            OP_FORALL = "forall";
            OP_EXISTS = "exists";
            OP_FREE   = "free";
            OP_LIST   = "list";
        }

        #define FPROTO_Parser
        #include "parser.proto"
        #undef FPROTO_Parser
    };

    /**************************************************************/

    enum {
        INT_TOK_TYPE = 1,
        IDFR_TOK_TYPE,
        QUOT_TOK_TYPE,
        OTHER_TOK_TYPE
    };

    /* String Array Pointer */
    struct sap {
        char* p;    // pointer
        int n;      // size
    };

    class Lexer {
        public:
        const char* filename;
        FILE* infile;
        sap CurLine;
        sap CurTok;
        int CurPos;
        int LineNum;
        int CurTokType;

        #define FPROTO_Lexer
        #include "parser.proto"
        #undef FPROTO_Lexer

        /* Constructor */
        Lexer(const char* filename_) : filename(filename_) {
            init();
        }

    };

};
