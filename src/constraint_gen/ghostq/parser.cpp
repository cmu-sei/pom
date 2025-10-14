/*****************************************************************************
* Lexer/Parser for GhostQ format.
* Copyright (C) 2010-2012 Will Klieber
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in 
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <vector>
#include <map>
using namespace std;

#include "parser.hh"
using namespace GhostQ_Parser;


#define talloc(type, n) ((type*) calloc(n, sizeof(type)))
#define LEX_ERR(str) {fprintf(stderr, "%s\n", str); abort();}


static inline int StrEqc(const char* s1, const char* s2) {
    // Precond: At least one of the strings is not null.
    if ((s1 == NULL) ^ (s2 == NULL)) {return 0;}
    return (strcasecmp(s1, s2) == 0);
}

static char* StrFmt(const char* fmt, ...) { 
    va_list ap;
    char* pOut = talloc(char, 16*1024);
    va_start(ap, fmt);
    vsprintf(pOut, fmt, ap);
    va_end(ap);
    return pOut;
}


/*****************************************************************************
* Lexer Stuff
*****************************************************************************/

void Lexer::init() {
    CurLine = (sap){NULL, 0};
    CurTok  = (sap){NULL, 0};
    CurPos = 0;
    LineNum = 0;
    if (StrEqc(filename, "-")) {
        infile = stdin;
    } else {
        infile = fopen(filename, "r");
    }
    if (infile == 0) {
        LEX_ERR(StrFmt("File '%s' does not exist.", filename));
    }
    AdvLine();
    AdvTok();
    while (StrEqc(CurTok.p, "\n")) {AdvTok();}
};

void Lexer::AdvLine() {
    CurLine.n = getline(&CurLine.p, (size_t*)&CurLine.n, infile);
    CurPos = 0;
    LineNum++;
}


void Lexer::AdvTok() {
    int InitPos;
    CurTokType = OTHER_TOK_TYPE;
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
        case '\r':  // For MS-DOS text files
            goto again;

        case ' ': case '\t': case ',':
            goto again;

        case '#':
            // Lines beginning with "#" are comment lines
            CurPos = CurLine.n - 1;
            goto again;

        case '\n': case '(': case ')': case '=': case ':':
            CurTok.n = 1;
            goto copytok;

        case '0' ... '9': case '-':
            CurTokType = INT_TOK_TYPE;
            while (true) {
              assert(CurPos <= CurLine.n);
              switch (CurLine.p[CurPos++]) {
                case '0' ... '9': continue;
                default: goto done_idfr;
              }
            }

        case 'a' ... 'z':
        case 'A' ... 'Z': case '_':
            CurTokType = IDFR_TOK_TYPE;
            while (true) {
              assert(CurPos <= CurLine.n);
              switch (CurLine.p[CurPos++]) {
                case '0' ... '9':  case 'a' ... 'z':  case 'A' ... 'Z':  case '_':
                case '-':  case '.':  case '@':  case '[':  case ']':
                case '<':  case '>':  case '*':
                  continue;
                default:
                  goto done_idfr;
              }
            }

        case '<': 
            CurPos++;
            goto done_idfr;

        done_idfr:
        CurPos--;
        CurTok.n = CurPos - InitPos;
        goto copytok;


        default: {
            char* pChrStr;
            unsigned char chr = CurLine.p[CurPos-1];
            if ((32 <= chr) && (chr <= 126)) {
                pChrStr = StrFmt("'%c'", chr);
            } else {
                pChrStr = StrFmt("0x%X", chr);
            }
            fprintf(stderr, "Bad character (%s) on line %i, column %i.\n",
                pChrStr, LineNum, CurPos);
            exit(1);
        }
    }
    copytok:
    CurTok.p = (char*) realloc(CurTok.p, CurTok.n+1);
    memcpy(CurTok.p, &CurLine.p[InitPos], CurTok.n);
    CurTok.p[CurTok.n] = 0;
        
}

char* Lexer::StrCurTokPos() {
    return StrFmt("line %i, col %i-%i", LineNum, CurPos - CurTok.n + 1, CurPos);
}

void Lexer::EatTok(const char* expected) {
    if (!StrEqc(CurTok.p, expected)) {
        LEX_ERR(StrFmt("Parsing error (%s): Expected '%s'.",
            StrCurTokPos(), expected));
    }
    AdvTok();
}

void Lexer::ReadLineSep() {
    if (!StrEqc(CurTok.p, "\n")) {
        LEX_ERR(StrFmt("Parsing error (%s): Expected a line break.", StrCurTokPos()));
    }
    while (StrEqc(CurTok.p, "\n")) {
        AdvTok();
    }
}

void Lexer::SkipLine() {
    AdvLine();
    AdvTok();
    while (StrEqc(CurTok.p, "\n")) {
        AdvTok();
    }
}
    

int Lexer::ReadInt() {
    if (CurTokType != INT_TOK_TYPE) {
        LEX_ERR(StrFmt("Parsing error (%s): Expected an integer.", StrCurTokPos()));
    }
    int ret = atoi(CurTok.p);
    AdvTok();
    return ret;
}

char* Lexer::ReadIdfr() {
    if (CurTokType != IDFR_TOK_TYPE) {
        LEX_ERR(StrFmt("Parsing error (%s): Expected an identifier.", StrCurTokPos()));
    }
    char* ret = strdup(CurTok.p);
    AdvTok();
    return ret;
}


vector<int> Lexer::ReadArgs() {
    vector<int> ret = vector<int>();
    EatTok("(");
    while (CurTokType == INT_TOK_TYPE) {
        ret.push_back(ReadInt());
    }
    if (StrEqc(CurTok.p, ")")) {
        AdvTok();
    } else {
        LEX_ERR(StrFmt("Parsing error (%s): Expected an integer or a ')'.",
            StrCurTokPos()));
    }
    return ret;
}

int Lexer::ReadIntParam(const char* pParamName) {
    EatTok(pParamName);
    int ret = ReadInt();
    ReadLineSep();
    return ret;
}



/*****************************************************************************
* Parser Stuff
*****************************************************************************/

static void DieParse(const char* pErrMsg, Lexer* lex, ...) {
    fprintf(stderr, "Error on line %i: ", lex->LineNum);
    va_list ap;
    va_start(ap, lex);
    vfprintf(stderr, pErrMsg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    abort();
}

const char* Parser::ReadOpIdfr(GhostQ_Parser::Lexer* pLex) {
    char* op = pLex->ReadIdfr();
    const char *(ops[]) = {OP_AND, OP_OR, OP_FORALL, OP_EXISTS, OP_FREE, OP_LIST};
    for (int i=0; i < (int)sizeof(ops); i++) {
        if (StrEqc(op, ops[i])) {
            free(op);
            return ops[i];
        }
    }
    LEX_ERR(StrFmt("Parsing error (%s): Unrecognized operator name '%s'.",
        pLex->StrCurTokPos(), pLex->CurTok.p));
}

void Parser::parse(const char* filename) {
    Lexer lex(filename);

    if (StrEqc(lex.CurTok.p, "CktQBF")) {lex.AdvTok(); lex.ReadLineSep();}
    this->LastInputVar = lex.ReadIntParam("LastInputVar");
    if (StrEqc(lex.CurTok.p, "FirstGateVar")) {lex.ReadIntParam("FirstGateVar");}
    this->LastGateVar = (lex.ReadIntParam("LastGateVar"));
    this->OutputGateLit = lex.ReadIntParam("OutputGateLit");
    if (StrEqc(lex.CurTok.p, "PreprocTimeMilli")) {
        this->PreprocTimeMilli = lex.ReadIntParam("PreprocTimeMilli");
    } else {
        this->PreprocTimeMilli = 0;
    }
    
    while (StrEqc(lex.CurTok.p, "VarName")) {
        lex.EatTok("VarName");
        int VarNum = lex.ReadInt();
        lex.EatTok(":");
        if (lex.CurTokType != IDFR_TOK_TYPE) {
            DieParse("Expected an identifier after the colon.", &lex, 0);
        }
        char* VarName = strdup(lex.CurTok.p);
        this->VarNames[VarNum] = VarName;
        lex.AdvTok();
        lex.ReadLineSep();
    }
    
    /* Read quantifier prefixes. */
    while (true) {
        if (lex.CurTok.p == NULL) {DieParse("Unexpected end-of-file.", &lex, 0);}
        if (lex.CurTokType == INT_TOK_TYPE) {break;}
        int qgate=0;
        sscanf(lex.CurLine.p, "<q gate=%u", &qgate);
        if (qgate == 0) DieParse("Expected a line beginning with \"<q gate=\" followed by a positive integer.", &lex, 0);
        lex.SkipLine();
        while (true) {
            if (strncmp(lex.CurLine.p, "</q>", 4) == 0) {
                lex.SkipLine();
                break;
            }
            char qtype=0;
            if      (StrEqc(lex.CurTok.p, "a")) {qtype = 'a';}
            else if (StrEqc(lex.CurTok.p, "e")) {qtype = 'e';}
            else if (StrEqc(lex.CurTok.p, "f")) {qtype = 'f';}
            else {DieParse("Expected 'a' or 'e' or 'f', but found '%.1s'.", &lex, lex.CurTok.p);}
            lex.AdvTok();

            vector<int> qvars = vector<int>();
            while (lex.CurTokType == INT_TOK_TYPE) {
                int qvar = lex.ReadInt();
                if (qvar <= 0) {DieParse("Variable number is not a positive integer.", &lex, 0);}
                if (qvar > LastInputVar) {DieParse("Variable number is out of range.", &lex, 0);}
                qvars.push_back(qvar);
            }
            lex.ReadLineSep();

            this->GateQuants[qgate].push_back(PrefixBlock(qtype, qvars));
        }
    }

    /* Read gate definitions */
    map<int, vector<int> > list_defs = map<int, vector<int> >();
    while (lex.CurTok.p != NULL) {
        int gate = lex.ReadInt();   
        if (gate <= 0) {DieParse("Gate number must be a positive integer.", &lex, 0);}
        lex.EatTok("=");
        const char* sOp = this->ReadOpIdfr(&lex);
        vector<int> args;
        if (sOp == OP_FORALL || sOp == OP_EXISTS) {
            lex.EatTok("(");
            int subgate = lex.ReadInt();
            if (abs(subgate) >= gate) {
                DieParse("Child gate numbers must be less than parent gate number.", &lex, 0);
            }
            if (list_defs.count(subgate) == 1) {
                args = list_defs[subgate]; // This does a deep copy of the vector.
                subgate = lex.ReadInt();
                args.insert(args.begin(), subgate);
            } else {
                args = lex.ReadArgs();
                args.insert(args.begin(), subgate);
            }
            lex.EatTok(")");
        } else {
            args = lex.ReadArgs();
        }
        if (sOp == OP_LIST) {
            list_defs[gate] = args;
        } else {
            this->GateDefs[gate] = GateDef(sOp, args);
        }
        lex.ReadLineSep();
    }
}

