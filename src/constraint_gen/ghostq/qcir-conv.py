#!/usr/bin/python

##############################################################################
# qcir-conv.py: Converts QDIMACS to QCIR or GhostQ format.
# Author: Will Klieber
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
##############################################################################

import sys
import os
import commands
import re
import pdb
import textwrap
import timeit
import StringIO
import random
from pprint import pprint
from random import Random
from array import array
from copy import copy 
import heapq
from math import sqrt

PrStage = True  # Print timing info for each stage.

try:
    from collections import OrderedDict
    PreferablyOrderedDict = OrderedDict
except:
    PreferablyOrderedDict = dict
    sys.stderr.write("Warning: OrderedDict not found, falling back to unordered dict!\n")

################################################################################
# Utility functions
################################################################################

def ite(test, tbra, fbra):
    #return (tbra if test else fbra)
    if test:
        return tbra
    else:
        return fbra

def die(text): 
    sys.stderr.write("Error in function '%s' at line %d:\n" % 
        (sys._getframe(1).f_code.co_name, sys._getframe(1).f_lineno))
    sys.stderr.write("\nERROR: " + text+"\n")
    if glo.Debug:
        pdb.set_trace()
    else:
        sys.exit(1)

def qdie(text): 
    sys.stderr.write(text + "\n")
    sys.exit(1)

stop = pdb.set_trace
stderr = sys.stderr

timer = timeit.default_timer
start_time = timer()
elapsed_time = (lambda: timer() - start_time)

irange = lambda start, end: range(start, end+1)

def unique(coll):
    hit = set()
    for x in coll:
        if x in hit:
            continue
        hit.add(x)
        yield x

def unique_by_equiv_class(coll, fn_ec):
    hit = set()
    for x in coll:
        ec = fn_ec(x)
        if ec in hit:
            continue
        hit.add(ec)
        yield x

def unique_conseq(it):
    try:
        cur = it.next()
        yield cur
        prev = cur
        while True:
            cur = it.next()
            if cur != prev:
                yield cur
                prev = cur
    except StopIteration:
        return

def unique_conseq_list(L):
    return list(unique_conseq(iter(L)))

class memoized(object):
    def __init__(self, func):
        self.func = func
        self.cache = {}
    def __call__(self, *args):
        try:
            return self.cache[args]
        except KeyError:
            value = self.func(*args)
            self.cache[args] = value
            return value

################################################################################

class LineReader:
    """Reads a file line-by-line."""
    def __init__(self, file):
        self.file = file
        self.cur = None
        self.line_num = 0
        self.advance()

    def advance(self):
        self.cur = self.file.readline()
        self.line_num += 1

    def skip_blank(self):
        while 1:
            if len(self.cur)==0:  # End-Of-File
                return
            self.cur = self.cur.strip()
            if len(self.cur)==0: 
                self.advance()
                continue
            return
        
    def skip_comment(self):
        while 1:
            if len(self.cur)==0:  # End-Of-File
                return
            self.cur = self.cur.strip()
            if self.cur.startswith('#') or len(self.cur)==0: 
                self.advance()
                continue
            return
        
    def close(self):
        self.file.close()


################################################################################

class ProbInst(object):
    
    def __init__(self):
        self.RepTable = None    # Maps each gate to expression defining it.
        self.QuantPfx = None    # Maps each gate to a list of QuantBlks.
        self.OutGlit = None     # Output gate literal of the circuit.
        # The following are derived from the above.
        self.QtypeFromVar = None # Maps an input var to 'a' or 'e'.
        self.LevByVar = None    # Maps each variable to its nesting level.
        self.VarsByLev = None   # Maps a nesting level to vars in it.
        self.VarPars = None     # Maps a var to the gates it feeds.  See FindParents.
        self.OccurPol = None

    def DumpRepTable(self):     # For debugging.
        for (gvar, expr) in sorted(self.RepTable.items()):
            print "RepTable[%i] = %s" % (gvar, repr(expr))

    def IsGateLit(self, x):
        return (abs(x) in self.RepTable)

    def NewGvar(self):
        self.NumVars += 1
        ret = self.NumVars
        return ret

    def get_list_new_to_old(self):
        p = self
        has_unnamed = False
        has_renamed = False
        ret = []
        for (NewNum, OldNum) in sorted(p.MapNewToOld.items()):
            if (NewNum in p.RepTable): 
                continue
            OrigName = p.OrigNames.get(OldNum, str(OldNum))
            if (OrigName.startswith(glo.VarNumPfx)):
                has_renamed = True
            if (OrigName[0] in "0123456789"):
                has_unnamed = True
                OrigName = glo.VarNumPfx + OrigName
            if (has_unnamed and has_renamed):
                if (glo.VarNumPfx == ""):
                    qdie("Error: Not expecting numeric variable names ('-var-num-pfx \"\"').")
                else:
                    qdie("Bad value for option '-var-num-pfx'.")
            ret.append((NewNum, OrigName))
        return ret
        

class QuantBlk:
    def __init__(self, qtype, qvars):
        self.qtype = qtype
        self.qvars = qvars

    def __repr__(self):
        return "QuantBlk(" + repr(self.qtype) + ", " + repr(self.qvars) + ")"


def read_input_file(filename):
    try:
        if (filename == '-'):
            file_ptr = sys.stdin
        elif (filename.endswith(".gz")):
            import gzip
            file_ptr = gzip.open(filename, 'r')
        else:
            file_ptr = open(filename, 'r')
    except IOError, e:
        qdie(str(e))
    in_file = LineReader(file_ptr)

    global start_time
    start_time = timer()
    in_file.skip_blank()
    FirstLine = in_file.cur.strip()

    if (FirstLine.startswith(("c")) or FirstLine.startswith("p cnf")):
        return read_qdimacs_file(in_file)

    if filename.endswith(".qcir") or FirstLine.lower().startswith("#qcir"):
        return read_qcir_file(in_file)

    for prefix in ["#", "LastInputVar", "CktQBF"]:
        if FirstLine.lower().startswith(prefix.lower()):
            return read_ghostq_file(in_file)

    die("Unrecognized file format.")


############################################################
def read_ghostq_file(in_file):
    def LineDie(msg):
        stderr.write("Error on line %i. " % in_file.line_num)
        stderr.write(msg + "\n")
        sys.exit(1)
    def ReadIntParam(pname):
        try:
            ret = int(re.match(pname + ' +(-?[0-9]+)\s*', in_file.cur).group(1))
            in_file.advance()
        except:
            die("Error trying to read '%s' parameter on line %i." % 
                    (pname, in_file.line_num))
        return ret
    prob = ProbInst()
    prob.OrigNames = {}
    in_file.skip_comment()
    if (in_file.cur.startswith("CktQBF")): in_file.advance()
    in_file.skip_comment()
    if (in_file.cur.startswith("LastInputVar")): ReadIntParam("LastInputVar")
    if (in_file.cur.startswith("FirstGateVar")): ReadIntParam("FirstGateVar")
    prob.NumVars = ReadIntParam("LastGateVar")
    prob.NumOrigVars = prob.NumVars
    prob.OutGlit = ReadIntParam("OutputGateLit")
    if (in_file.cur.startswith("PreprocTimeMilli")): ReadIntParam("PreprocTimeMilli")
    while True:
        in_file.skip_comment()
        if not(in_file.cur.startswith("VarName ")):
            break
        m = re.match('VarName +([0-9]+)\s*:\s*([*-z]+)\s*$', in_file.cur)
        if (not m):
            LineDie("Error reading VarName line.")
        glo.KeepVarNames = True
        prob.OrigNames[int(m.group(1))] = m.group(2)
        in_file.advance()
        
    prob.NumElim = 0
    prob.QuantPfx = {}
    prob.RepTable = {}
    NumVarsDecl = prob.NumVars

    glo.input_format = "cqbf"

    HasAnyQ = False

    while True:
        in_file.skip_comment()
        CurLine = in_file.cur
        if (CurLine == "<q>") and not HasAnyQ:
            GateNum = abs(prob.OutGlit)
        else:
            try: 
                GateNum = int(re.match('<q gate=(-?[0-9]+)>', CurLine).group(1))
            except: 
                if (re.match('^[ae]', CurLine.strip())):
                    LineDie("Expected a line of the form '<q>' or '<q gate=i>'.")
                break
        if GateNum in prob.QuantPfx:
            LineDie("Input file has two occurrences of '<q gate=%i>'." % GateNum)
        if GateNum < 0:
            LineDie("Gate number cannot be negative.")
        HasAnyQ = True
        QuantBlks = []
        while True:
            in_file.advance()
            in_file.skip_comment()
            if len(in_file.cur)==0: LineDie("File ended unexpectedly.")
            CurLine = in_file.cur
            if (CurLine[0].lower() not in ['a','e','f']): break
            CurQuant = QuantBlk(CurLine[0], [int(x) for x in CurLine[1:].split()])
            QuantBlks.append(CurQuant)
        if (CurLine != "</q>"): LineDie("Expected '</q>'.")
        in_file.advance()
        prob.QuantPfx[GateNum] = QuantBlks

    def MakeXor(NewGate, args):
        if len(args)==1:
            return args[0]
        if (NewGate == 0):
            NewGate = prob.NumVars+1
            prob.NumVars += 1
        temp1 = prob.NumVars+1
        temp2 = prob.NumVars+2
        prob.NumVars += 2
        x = args[0]
        y = MakeXor(0, args[1:])
        prob.RepTable[temp1] = tuple(['or', x, y])
        prob.RepTable[temp2] = tuple(['or', -x, -y])
        prob.RepTable[NewGate] = tuple(['and', temp1, temp2])
        return NewGate

    VarSets = {}
    while True:
        if len(in_file.cur)==0: break
        CurLine = in_file.cur.strip().lower()
        if CurLine.startswith('#') or len(CurLine)==0: 
            in_file.advance()
            continue
        try:
            [GateVar, op, args] = re.match('([0-9]+) += +([a-zA-Z]+)\((.*)\)', in_file.cur).groups()
            GateVar = int(GateVar)
            if GateVar > NumVarsDecl:
                die("Line %i: Variable number (%i) exceeds maximum declared (%i)." %
                    (in_file.line_num, GateVar, NumVarsDecl))
            args = [int(x) for x in args.replace(",", " ").split()]
            if (op == "true"):
                assert(len(args) == 0)
                op = "and"
            if (op == "false"):
                assert(len(args) == 0)
                op = "or"
            if (op in ["and", "or"]):
                expr = tuple([op] + args)
                prob.RepTable[GateVar] = expr 
            elif (op == 'xor'):
                MakeXor(GateVar, args)
            elif (op == 'list'):
                VarSets[GateVar] = args
            elif (op in ["exists", "forall", "free"]):
                if len(args) != 2:
                    die("Line %i: Quantifier requires exactly two args." % (in_file.line_num));
                if (args[0] not in VarSets):
                    die("Line %i: First arg to quantifier must be previously defined list." % (in_file.line_num));
                if GateVar in prob.QuantPfx:
                    die("Line %i: Gate is already quantified in prefix." % (in_file.line_num));
                QuantLetr = {'exists':'e', 'forall':'a', 'free':'f'}[op]
                prob.QuantPfx[GateVar] = [QuantBlk(QuantLetr, VarSets[args[0]])]
                expr = tuple(["and"] + [args[1]])
                prob.RepTable[GateVar] = expr 
            else:
                die("Unrecognized operator: '%s'" % op)
        except Exception, e:
            die("Error trying to parse line %i." % (in_file.line_num))
        in_file.advance()

    in_file.close()
    if (PrStage): sys.stderr.write("Done reading file, %.4f sec\n" % elapsed_time())
    return prob

############################################################
def read_qcir_file(in_file):
    def LineDie(msg):
        stderr.write("Error on line %i. " % in_file.line_num)
        stderr.write(msg + "\n")
        sys.exit(1)
    def ReadIntParam(pname):
        try:
            ret = int(re.match(pname + ' +(-?[0-9]+)\s*', in_file.cur).group(1))
            in_file.advance()
        except:
            die("Error trying to read '%s' parameter on line %i." % 
                    (pname, in_file.line_num))
        return ret
    prob = ProbInst()
    prob.OrigNames = {}
    if glo.KeepVarNames == True:
        while True:
            in_file.skip_blank()
            if not(in_file.cur.startswith("#VarName ")):
                if in_file.cur.startswith("#"):
                    in_file.advance()
                    continue
                else:
                    break
            m = re.match('#VarName +([0-9]+)\s*:\s*([*-z]+)\s*$', in_file.cur)
            if (not m):
                LineDie("Error reading VarName line.")
            prob.OrigNames[int(m.group(1))] = m.group(2)
            in_file.advance()
    in_file.skip_comment()
        
    prob.NumElim = 0
    prob.QuantPfx = {}
    prob.RepTable = {}

    glo.input_format = "qcir"

    HasAnyQ = False

    QuantBlks = []
    output_var = None
    while True:
        in_file.skip_comment()
        CurLine = in_file.cur.strip()
        in_file.advance()
        m = re.match('^([A-Za-z]+)[(](.*)[)]$', CurLine)
        if not m:
            LineDie("Expected a line of the form 'quant(var_list)' or 'output(var)'.")
        [qtype, var_list] = m.groups()
        qtype = qtype.lower()
        if qtype == 'output':
            prob.OutGlit = int(var_list)
            if prob.OutGlit < 0:
                LineDie("Negated output is not supported!")
            break
        try:
            qtype = {'exists':'e','forall':'a','free':'f'}[qtype]
        except KeyError:
            LineDie("Unrecognized token: '%s'." % (qtype,))
        var_list = [int(x) for x in var_list.replace(',', ' ').split()]
        QuantBlks.append(QuantBlk(qtype, var_list))

    prob.QuantPfx[prob.OutGlit] = QuantBlks

    VarSet = set()
    while True:
        if len(in_file.cur)==0: break
        CurLine = in_file.cur.strip().lower()
        if CurLine.startswith('#') or len(CurLine)==0: 
            in_file.advance()
            continue
        try:
            [GateVar, op, args] = re.match('([0-9]+) += +([a-zA-Z]+)\((.+)\)', in_file.cur).groups()
            GateVar = int(GateVar)
            args = [int(x) for x in args.replace(",", " ").split()]
            VarSet = VarSet.union(abs(x) for x in (args + [GateVar]))
            if (op in ["and", "or"]):
                expr = tuple([op] + args)
                prob.RepTable[GateVar] = expr 
            else:
                die("Unrecognized operator: '%s'" % op)
        except Exception, e:
            die("Error trying to parse line %i." % (in_file.line_num))
        in_file.advance()
    
    prob.NumVars = max(VarSet)
    prob.NumOrigVars = prob.NumVars
    NumVarsDecl = prob.NumVars

    in_file.close()
    if (PrStage): sys.stderr.write("Done reading file, %.4f sec\n" % elapsed_time())
    return prob

############################################################
def read_qdimacs_probline(cnf_file):
    # Skip comment lines
    while cnf_file.cur.startswith("c") or cnf_file.cur == "\n":
        if glo.KeepVarNames and cnf_file.cur.startswith("c VarName "):
            m = re.match('c VarName +([0-9]+)\s*:\s*([*-z]+)\s*$', cnf_file.cur)
            if m:
                glo.OrigNames[int(m.group(1))] = m.group(2)
        cnf_file.advance()
    # Read the problem line
    try:
        [NumVars, NumClauses] = [int(num) for num in 
                re.match('p cnf +([0-9]+) +([0-9]+)\s*', cnf_file.cur).groups()]
        return [NumVars, NumClauses]
    except:
        die("Error reading the problem line.")

def read_qdimacs_file(cnf_file):
    glo.input_format = "qdimacs"
    
    # Read the problem line
    prob_line = cnf_file.cur
    [NumVars, NumClauses] = read_qdimacs_probline(cnf_file)
    cnf_file.advance()
    
    # Read the quantifiers
    QuantBlks = []
    while len(cnf_file.cur) > 0 and cnf_file.cur[0].lower() in ['a', 'e', 'f']:
        qtype = cnf_file.cur[0].upper()
        try:
            qvars = [int(lit) for lit in cnf_file.cur[1:].split()]
        except:
            die("Error reading quantifier on line #%d." % (cnf_file.line_num))
        if qvars[len(qvars)-1] != 0:
            die("Error: Line %i does not end in a 0." % cnf_file.line_num)
        for var in qvars:
            if var > NumVars:
                die("Line %i: Variable number (%i) exceeds maximum declared (%i)." %
                    (cnf_file.line_num, var, NumVars))
        QuantBlks.append( QuantBlk(qtype, qvars[0:len(qvars)-1]) )
        cnf_file.advance()

    # Support implicit existential block
    if QuantBlks == [] and NumVars > 0:
        QuantBlks.append(QuantBlk('E', irange(0, NumVars)))

    # Read the clauses
    HasVar = array('B', [False] * (NumVars+1))
    Clauses = [None] * NumClauses
    warn_dup_lits = True
    warn_contra_lits = True
    for i in range(0, NumClauses):
        try:
            cur = tuple([int(lit) for lit in cnf_file.cur.split()])
        except:
            die("Error reading clause #%d:\n%s" % (i, cnf_file.cur))
        if len(cur) == 0: die("Fewer clauses exist than declared (<%i)." % NumClauses)
        if cur[len(cur)-1] != 0:
            die("Error: Line %i does not end in a 0." % cnf_file.line_num)
        cur = cur[0:len(cur)-1]  # Remove terminating "0"
        for var in (abs(x) for x in cur):
            if var > NumVars:
                die("Line %i: Variable number (%i) exceeds maximum declared (%i)." %
                    (cnf_file.line_num, var, NumVars))
            HasVar[var] = True
        old_cur = cur
        cur = tuple(unique(cur))
        if old_cur != cur and warn_dup_lits:
            stderr.write("*** Duplicate literals (in file %s) in clause on line %i! ***\n" % (glo.filename, cnf_file.line_num))
            warn_dup_lits = False
        if (set(-x for x in cur) & set(cur)) and warn_contra_lits:
            stderr.write("*** Contradictory literals (in file %s) in clause on line %i! ***\n" % (glo.filename, cnf_file.line_num))
            cur = None
            warn_contra_lits = False
        Clauses[i] = cur
        cnf_file.advance()
    Clauses = [cl for cl in Clauses if not(cl is None)]
    while cnf_file.cur.startswith("c") or cnf_file.cur == "\n":
        cnf_file.advance()
    if cnf_file.cur.strip() != "": die("More clauses exist than declared (>%i)." % NumClauses)
    NumOrigVars = 0
    for var in range(1, NumVars+1):
        if HasVar[var]:
            NumOrigVars += 1

    cnf_file.close()

    # Check that all variables are quantified
    HasQuant = array('B', [False] * (NumVars+1))
    for qb in QuantBlks:
        for var in qb.qvars:
            if HasQuant[var]:
                die("Variable %i quantified more than once!" % var)
            HasQuant[var] = True

    for var in range(1, NumVars+1):
        if HasVar[var] and not HasQuant[var]:
            if QuantBlks[0].qtype.lower() == 'e':
                sys.stderr.write(("WARNING: Variable %i isn't explicitly quantified" + ("!" * 80) + "\n") % var)
                QuantBlks[0].qvars.append(var)
            else:
                sys.stderr.write("\nERROR: Variable %i isn't explicitly quantified!\n" % var)
                qdie("")

    UsedVars = [x for x in range(1, NumVars+1) if HasVar[x]]
            

    return cnf_to_nnf(UsedVars, NumVars, QuantBlks, Clauses)


    


############################################################
def cnf_to_nnf(UsedVars, NumVars, QuantBlks, Clauses):
    orig_args = (UsedVars, NumVars, QuantBlks, Clauses)
    NumOrigVars = len(UsedVars)
    prob = ProbInst()
    prob.NumOrigVars = NumOrigVars
    prob.NumVars = NumVars

    NumOrigClauses = len(Clauses)
    if (PrStage): sys.stderr.write("Done reading file (%i vars, %i clauses), %.4f sec\n" % 
        (NumOrigVars, NumOrigClauses, elapsed_time()))

    UniqueClauses = list(unique(Clauses))
    if len(UniqueClauses) != len(Clauses):
        stderr.write("*** Repeated clauses in file %s! ***\n" % (glo.filename))
        Clauses = UniqueClauses
    UniqueClauses = None
        

    QbFromVar = {} 
    #QbFromVar = array('l', [999999] * (NumVars+1))
    LocQtypeFromVar = {}
    #LocQtypeFromVar = array('c', ['x'] * (NumVars+1))
    evars = []
    for (ixQb, qb) in enumerate(QuantBlks):
        for v in qb.qvars:
            QbFromVar[v] = ixQb
            LocQtypeFromVar[v] = qb.qtype
            if qb.qtype.lower() == 'e':
                evars.append(v)
    
    RepTable = {}

    def handle_cycle(cycle):
        if glo.LtdRevEngr:
            stderr.write("Cycle: %r\n" % (cycle,))
            qdie("Error (file %s): Cycle found in RepTable; aborting!" % (glo.filename,))
        #print cycle
        #stop()
        stderr.write("Cycle: %r\n" % (cycle,))
        sys.stderr.write("WARNING (file %s): Cycle found in RepTable; restarting with limited reverse engineering...\n" % (glo.filename,))
        glo.LtdRevEngr = True
        Clauses = None
        RepTable = None
        return cnf_to_nnf(*orig_args)

    #########################################################################
    if 1 and len(QuantBlks) == 3 and QuantBlks[1].qtype.lower() == 'a' and (4 <= len(QuantBlks[1].qvars) <= 6):
        stderr.write("Considering file %s\n" % (glo.filename))
        ImpDefs = {}
        UnivVars = tuple(QuantBlks[1].qvars)
        TotUniv = len(UnivVars)
        for CurCl in Clauses:
            if not (TotUniv + 1 <= len(CurCl) <= TotUniv + 2):
                continue
            inner = None
            other = []
            cancel = False
            NumUniv = 0
            for lit in CurCl:
                if QbFromVar[abs(lit)] == 2:
                    if (inner is None):
                        inner = lit
                    else:
                        cancel = True
                        break
                else:
                    if QbFromVar[abs(lit)] == 1:
                        NumUniv += 1
                    other.append(lit)
            if cancel or (len(QuantBlks[1].qvars) != NumUniv):
                continue
            ImpDefs.setdefault(inner, set())
            ImpDefs[inner].add(tuple(sorted([-x for x in other], key=abs)))

        VarsCovered = []
        DelClauses = set()

        for (RepLit, ImpConjs) in ImpDefs.items():
            if RepLit < 0: continue
            if -RepLit not in ImpDefs: continue
            covered = ImpDefs[RepLit] | ImpDefs[-RepLit]
            flag = True
            while flag:
                flag = False
                old_covered = copy(covered)
                discard = set()
                ResolvOrigin = {}
                for conj in copy(covered):
                    for lit in conj:
                        mate = tuple((x if x != lit else -x) for x in conj)
                        if mate in covered:
                            resolvent = tuple(x for x in conj if x != lit)
                            discard.add(conj)
                            discard.add(mate)
                            covered.add(resolvent)
                            ResolvOrigin[resolvent] = (conj, mate)
                covered -= discard
                if covered != old_covered:
                    flag = True
            #print covered
            if len(covered)==1 and (() in covered):
                VarsCovered.append(RepLit)
                SubGates = []
                for ImpConj in ImpDefs[RepLit]:
                    CurGate = prob.NewGvar()
                    RepTable[CurGate] = (('and',) + ImpConj)
                    SubGates.append(CurGate)
                RepTable[RepLit] = (('or',) + tuple(SubGates))
                # Note: xsub isn't needed because none of the children are eligible to be gates.

                def ClauseFromImpDef(conj, rep):
                    return tuple(sorted([rep] + [-x for x in conj]))
                DelClauses |= (set(ClauseFromImpDef(imp, RepLit) for imp in ImpDefs[RepLit]) |
                               set(ClauseFromImpDef(imp, -RepLit) for imp in ImpDefs[-RepLit]))

        if DelClauses:
            stderr.write("Got it (file %s): %d clauses\n" % (glo.filename, len(DelClauses)))
        Clauses = [cl for cl in Clauses if (tuple(sorted(cl)) not in DelClauses)]
                
        del ImpDefs

        NumVars = prob.NumVars





    # Compute which vars are eligible to be representative vars
    if QuantBlks == []:
        elig = array('B', [True] * (NumVars+1))
    else:
        elig = array('B', [False] * (NumVars+1))
        inr = QuantBlks[-1]  # innermost quantifier
        if (inr.qtype.lower() == 'e'):
            for var in inr.qvars:
                if var > NumVars:
                    die("Bad var number %i: exceeds number of declared vars." % var)
                elig[var] = True
        else: 
            assert(inr.qtype.lower() in ['a','f'])

    DelClauses = set()

    @memoized 
    def HasDescendent(gate, targ):
        assert(gate > 0)
        assert(targ > 0)
        if gate == targ:
            return True
        defn = RepTable.get(gate, None)
        if defn is None: 
            return False
        assert(defn[0] in ('and','or'))
        return any(HasDescendent(abs(x), targ) for x in defn[1:])

    t_round = 0
    pNumElim = [0]
    #xsub = [set() for var in range(0, NumVars+1)]
    xsub = dict([(var, set()) for var in range(0, NumVars+1)])
    #glo.xsub = xsub  #DEBUG!
    
    class LitByteArr(object):
        def enc(self, lit):
            return (abs(lit) << 1) | (lit < 0)
        def __init__(self, default):
            self.data = array('B', [default] * (NumVars+1)*2)
        def __getitem__(self, lit):
            return self.data[self.enc(lit)]
        def __setitem__(self, lit, val):
            self.data[self.enc(lit)] = val

    class LitArr(object):
        def __init__(self, fnDefault):
            self.data = [fnDefault() for x in range(0, ((NumVars+1)*2))]
        def __getitem__(self, lit):
            return self.data[(abs(lit) << 1) | (lit < 0)]
        def __setitem__(self, lit, val):
            self.data[(abs(lit) << 1) | (lit < 0)] = val

    
    #########################################################################
    def calc_extra_xsub(SubLits):
        extra_xsub = set()
        for CurLit in SubLits:
            extra_xsub |= xsub[abs(CurLit)]
        for CurLit in SubLits:
            if (abs(CurLit) not in RepTable) and elig[abs(CurLit)]:
                extra_xsub.add(abs(CurLit))
        return extra_xsub

    def TryRepLit(RepLit, SubLits):
        has_cycle = False
        for CurLit in SubLits:
            if glo.LtdRevEngr and abs(CurLit) < abs(RepLit):
                return False
            if abs(RepLit) in xsub[abs(CurLit)]:
                #assert(HasDescendent(abs(CurLit), abs(RepLit)))
                has_cycle = True
                #stderr.write("CYCLE!  " + str(abs(RepLit)) + "\n")
            else:
                #assert(not HasDescendent(abs(CurLit), abs(RepLit)))
                pass
        if has_cycle: return False
        #
        if t_round==1 and len(SubLits) == 1:
            return False
        #
        if abs(RepLit) in RepTable: 
            if (t_round == 2):
                return False
            print "WARNING: It seems gate %i has two definitions." % RepLit
        #
        pNumElim[0] += 1
        xsub[abs(RepLit)] = calc_extra_xsub(SubLits)
        #
        [pos_op, neg_op] = ['and', 'or']
        if RepLit > 0: RepTable[RepLit] = tuple([pos_op] + [-lit for lit in SubLits])
        if RepLit < 0: RepTable[-RepLit] = tuple([neg_op] + SubLits)
        elig[abs(RepLit)] = False
        #
        for CurLit in SubLits:
            CurDel = paired[-RepLit][-CurLit]
            if len(CurDel) == 2:
                ModDel = CurDel
            else:
                companions = var_to_companions.get(abs(RepLit), ())
                ModDel = [x for x in CurDel if (x not in companions)]
            assert(set(ModDel) == set((-RepLit, -CurLit)))
            assert(CurDel in ClauseSet)
            DelClauses.add(CurDel)
            if ModDel != CurDel:
                companion_influenced_gates.append(RepLit)
        #sys.stderr.write("RepLit %i\n" % RepLit)
        return True

    
    #########################################################################

    if len(Clauses) > glo.MonoClauseLimit and NumOrigVars > glo.MonoVarLimit:
        glo.NoMonoRep = True
    
    lit_to_def_clauses = {}

    if not glo.NoMonoRep:
        for cl in Clauses:
            for RepLit in cl:
                if LocQtypeFromVar[abs(RepLit)].lower() == 'e':
                    L = lit_to_def_clauses.setdefault(RepLit, [])
                    if L != ():
                        L.append(cl)
                    if len(lit_to_def_clauses[RepLit]) > 100 or len(cl) == 1:
                        lit_to_def_clauses[RepLit] = ()

    #if (PrStage): sys.stderr.write("Done computing lit_to_def_clauses, %.4f sec\n" % elapsed_time())


    #########################################################################
    # If a literal x always accompanies a variable v (i.e., if for all clauses C,
    # ((v in C) or (~v in C)) implies (x in C)), then var_to_companions[v] 
    # should contain x.
    # As an example of where this is useful, see adder-4-unsat.qdimacs,
    # available at the below URL:
    # http://www.qbflib.org/APPLICATION/qbflib_repository/Benchmarks/Ayari/Adder/adder-4-unsat.qdimacs.gz
    # In this benchmark instance, var_to_companions[510] = set([-731, -671, -630, -628]).
    # This means that value of variable 510 is a don't-care unless all of the
    # literals [-731, -671, -630, -628] are false.
    
    var_to_companions = {}
    companion_influenced_gates = []
    if not glo.NoCompanions:
        for cl in Clauses:
            for RepLit in cl:
                if LocQtypeFromVar[abs(RepLit)].lower() == 'e':
                    if len(cl) > 100:
                        # Avoid slowdown on instances like s499_d24_u.qdimacs
                        cur_companions = set()
                    else:
                        cur_companions = set(x for x in cl if x != RepLit)
                    rep = abs(RepLit)
                    S = var_to_companions.setdefault(rep, cur_companions)
                    if S != ():
                        S &= cur_companions
                        if len(var_to_companions[rep]) == 0:
                            var_to_companions[rep] = ()
                            
    num_vars_with_companions = len([v for v in UsedVars if var_to_companions.get(v, ())])
    if (PrStage): sys.stderr.write("Done finding companions (%i vars with companions), %.4f sec\n" % (num_vars_with_companions, elapsed_time()))

            
    #########################################################################
    
    #only_paired = LitByteArr(True)
    has_lit_pol = LitByteArr(False)
    num_clauses_with_var = dict((i, 0) for i in UsedVars)
    num_clauses_with_lit = dict([(i, 0) for i in UsedVars] + 
                                [(-i, 0) for i in UsedVars])

    # Compute paired literals
    paired = {}
    triples = {}
    halfdefs = {}
    fulldefs = {}

    def ComputePairing():
        for var in range(1, NumVars+1):
            paired[var] = dict()
            paired[-var] = dict()
        for lit in range(0, (NumVars+1)*2):
            #only_paired.data[lit] = True
            has_lit_pol.data[lit] = False
            
        for (ixCl, CurCl) in enumerate(Clauses):
            for lit in CurCl:
                enc_lit = (abs(lit) << 1) | (lit < 0)
                has_lit_pol.data[enc_lit] = True
                num_clauses_with_var[abs(lit)] += 1
                num_clauses_with_lit[lit] += 1
            ModCl = CurCl
            if len(CurCl) > 2:
                for RepLit in CurCl:
                    companions = var_to_companions.get(abs(RepLit), ())
                    if companions:
                        TestCl = [lit for lit in CurCl if (lit not in companions)]
                        if len(TestCl) == 2:
                            ModCl = TestCl
                            break
            if len(ModCl) == 2:
                paired[ModCl[0]][ModCl[1]] = CurCl
                paired[ModCl[1]][ModCl[0]] = CurCl
            #if False and len(ModCl) == 3:
            #    MaxQb = max(QbFromVar[abs(x)] for x in ModCl)
            #    for RepLit in ModCl:
            #        if QbFromVar[abs(RepLit)] != MaxQb:
            #            continue
            #        others = [x for x in ModCl if x != RepLit]
            #        triples.setdefault(RepLit, {})
            #        for (sel, a_val) in [(others[0], others[1]), (others[1], others[0])]:
            #            triples[RepLit].setdefault(sel, {})[a_val] = CurCl
            #            try:
            #                if -a_val in triples[-RepLit][sel]:
            #                    halfdefs.setdefault(abs(RepLit), {}).setdefault(sel, []).append(abs(a_val))
            #                if -sel in halfdefs[abs(RepLit)]:
            #                    fulldefs.setdefault(abs(RepLit), []).append(sel)
            #            except KeyError:
            #                pass
            ##else:
            ##    for lit in CurCl:
            ##        enc_lit = (abs(lit) << 1) | (lit < 0)
            ##        only_paired.data[enc_lit] = False

    ComputePairing()
    
    if (PrStage): sys.stderr.write("Done finding pairings, %.4f sec\n" % elapsed_time())

    #########################################################################
    # mono_e = set()
    # for qb in QuantBlks:
    #     if qb.qtype.lower() == 'a':
    #         continue
    #     for var in qb.qvars:
    #         if (has_lit_pol[var] and not has_lit_pol[-var]):
    #             mono_e.add(var)
    #         if (has_lit_pol[-var] and not has_lit_pol[var]):
    #             mono_e.add(-var)
    #
    # if (PrStage): sys.stderr.write("Done finding monotone lits (%i), %.4f sec\n" % 
    #     (len(mono_e), elapsed_time()))
    # Clauses = [cl for cl in Clauses if not any(lit in mono_e for lit in cl)]

     

    NumMonoRep = 0
    NotMonoElig = set()
    #glo.UseSeq = 0

    round_flag = True
    
    StatMod = (len(Clauses) // 72) + 1

    AllowGuess = False

    MonoRepSubst = {}

    #ALIAS_ROUND = (1 if not glo.NoVarAlias else -1)

    while (not glo.NoRevEngr) and t_round < 100:

        DelClauses = set()
        ClauseSet = set(Clauses)

        t_round += 1

        if (round_flag == False): break

        round_flag = False

        # (v <==> (x1 | x2 | x3)) expands to
        # (~v | x1 | x2 | x3)   &   (~x1 | v)  &  (~x2 | v)  &  (~x3 | v)
        #
        # (v <==> (x1 & x2 & x3)) expands to
        # (~x1 | ~x2 | ~x3 | v)  &  (~v | x1)  &  (~v | x2)  &  (~v | x3)

        if (PrStage): stderr.write("R%2i [" % t_round)

        for (ixCl, CurCl) in enumerate(Clauses):
            if ((ixCl % StatMod) == 0):
                if (PrStage): stderr.write(".")
            if len(CurCl) < 2:
                continue
            if CurCl in DelClauses:
                continue
            RepLits = []
            #print "CLAUSE ", CurCl
            for TestLit in CurCl:
                if (LocQtypeFromVar[abs(TestLit)].lower() != 'e'):
                    continue
                if abs(TestLit) in RepTable:
                    continue
                companions = var_to_companions.get(abs(TestLit), ())
                ModCl = tuple(lit for lit in CurCl if (lit not in companions))
                MaxQb = max(QbFromVar[abs(x)] for x in ModCl)
                if QbFromVar[abs(TestLit)] != MaxQb: 
                    assert( not elig[abs(TestLit)])
                    continue
                for CurLit in CurCl:
                    if CurLit == TestLit: continue
                    if CurLit in companions: continue
                    if -TestLit not in paired[-CurLit]:
                        break
                    if len([x for x in paired[-CurLit][-TestLit] if (x not in companions)]) != 2:
                        break
                    #if glo.UseSeq and t_round==2 and abs(ixCl - paired[-CurLit][-TestLit]) > len(CurCl):
                    #    break
                else:
                    RepLits.append(TestLit)
            if len(RepLits) == 0: continue
            if len(RepLits) != 1 and not AllowGuess:
                continue
            RepLit = RepLits[0]
            companions = var_to_companions.get(abs(RepLit), ())
            SubLits = [lit for lit in CurCl if lit != RepLit and (lit not in companions)]
            if len(SubLits) == 0: continue
            #
            got_it = TryRepLit(RepLit, SubLits)
            if got_it:
                DelClauses.add(CurCl)
                NotMonoElig |= (set(abs(x) for x in SubLits) | set([abs(RepLit)]))
                round_flag = True

        #stderr.write("RF %i\n" % round_flag)
        #if t_round == ALIAS_ROUND:
        #    round_flag = True

        MonoFlag = True
        SafeDel = set()
        while round_flag == False and (not AllowGuess) and MonoFlag and (not glo.NoMonoRep) and QuantBlks and QuantBlks[-1].qtype.lower() == 'e':
            assert(len(DelClauses) == 0)
            MonoFlag = False
            for RepVar in evars:
                if not (has_lit_pol[RepVar] and has_lit_pol[-RepVar]):
                    continue
                if RepVar in NotMonoElig:
                    continue
                for RepLit in [RepVar, -RepVar]:
                    if abs(RepLit) in RepTable:
                        continue
                    if len(lit_to_def_clauses.get(RepLit, [])) <= 1:
                        continue
                    ok = True
                    companions = var_to_companions[abs(RepLit)]
                    for cl in lit_to_def_clauses[RepLit]:
                        if cl in SafeDel: continue
                        if cl not in ClauseSet:
                            #continue
                            #lit_to_def_clauses[RepLit] = []
                            ok = False
                            break
                        for lit in cl:
                            if lit == RepLit or (lit in companions):
                                continue
                            #assert(elig[abs(lit)] == ((abs(lit) not in RepTable) and 
                            #                          (QbFromVar[abs(lit)] == len(QuantBlks) - 1)))
                            #if elig[abs(lit)]:
                            #    ok = False
                            #    break
                            if (abs(lit) in RepTable):
                                ok = False
                                break
                            if QbFromVar[abs(lit)] >= QbFromVar[abs(RepLit)]:
                                ok = False
                                break
                    if not ok:
                        continue
                    #stderr.write("%s: GOT ONE: %i\n" % (glo.filename, RepLit))
                    SubLits = []
                    for cl in lit_to_def_clauses[RepLit]:
                        if cl in SafeDel: 
                            continue
                        SafeDel.add(cl)
                        mod_cl = [lit for lit in cl if lit != RepLit and (lit not in companions)]
                        if len(mod_cl) == 1:
                            SubLits.append(-mod_cl[0])
                        else:
                            SubGate = prob.NewGvar()
                            RepTable[SubGate] = (('and',) + tuple(-lit for lit in mod_cl))
                            #xsub[SubGate] = calc_extra_xsub(mod_cl)
                            SubLits.append(SubGate)
                            #stderr.write("%i = and%r\n" % (SubGate, tuple(-lit for lit in mod_cl)))
                    if len(SubLits) == 0: 
                        continue
                    #expr = (('or',) + tuple(SubLits))
                    expr = (('or',) + (('protected', RepLit),) + tuple(SubLits))
                    RepGate = prob.NewGvar()
                    MonoRepSubst[abs(RepLit)] = RepGate
                    MonoRepSubst[-abs(RepLit)] = -RepGate
                    MonoRepSubst[('protected', RepLit)] = RepLit
                    MonoRepSubst[('protected', -RepLit)] = -RepLit
                    #stderr.write("%i = or%r\n" % (RepLit, tuple(SubLits)))
                    if RepLit > 0:
                        RepTable[RepLit] = expr
                    else:
                        RepTable[-RepLit] = NegateExpr(expr)
                    #xsub[abs(RepLit)] = calc_extra_xsub(SubLits) # Not needed because none are eligible to be gates.
                    elig[abs(RepLit)] = False
                    NumMonoRep += 1
                    MonoFlag = True
                    round_flag = True
                    #if only_paired[RepLit]:
                    #    SubLits = [-x for x in paired[RepLit]]
                    #    #stderr.write("NARF: " + str(RepLit) + "\n")
                    #    if TryRepLit(-RepLit, SubLits):
                    #        NumMonoRep += 1
        DelClauses |= SafeDel

        if round_flag == False and not AllowGuess:
            #stderr.write("AllowGuess := True\n")
            AllowGuess = True
            round_flag = True

        #if t_round == ALIAS_ROUND:
        #    Clauses = [tuple(SubAli(x) for x in clause) for clause in Clauses if (clause not in DelClauses)]
        #    ComputePairing()
        #else:
        Clauses = [clause for clause in Clauses if (clause not in DelClauses)]
        #print "DONE ROUND %i" % t_round
        if (PrStage): stderr.write("]\n")
        #pprint(RepTable)
        continue 


    #if (PrStage): stderr.write("\n")

    if (PrStage): sys.stderr.write("Done finding AND/OR gate variables (%3i + %i), %.4f sec\n" % 
        (len([x for x in RepTable if x in QbFromVar]) - NumMonoRep, NumMonoRep, elapsed_time()))
    
    if glo.PrintCompanionGates:
        if len(companion_influenced_gates) > 0:
            sys.stderr.write("Most companions: %i (gate %i)\n" % 
                max([(len(var_to_companions[abs(x)]), x) for x in companion_influenced_gates]))
        sys.stderr.write("Companion-influenced gates: %r\n" % sorted(set(companion_influenced_gates), key=abs))
    NumGatesPreMux = len(RepTable)

    ################################################################################

    for (OrigLit, RepLit) in MonoRepSubst.items():
        if isinstance(OrigLit, tuple) and OrigLit[0] == 'protected':
            continue
        if RepLit < 0:
            assert(OrigLit < 0)
            assert(-OrigLit in MonoRepSubst)
            continue
        RepTable[RepLit] = RepTable[OrigLit]
        xsub[abs(RepLit)] = xsub[abs(OrigLit)]
        del RepTable[OrigLit]

    #def subst_lits_in_tuple(cl, subst):
    #    return tuple([subst.get(x, x) for x in cl])

    #Clauses = [subst_lits_in_tuple(cl, MonoRepSubst) for cl in Clauses]
    Clauses = [tuple([MonoRepSubst.get(x, x) for x in cl]) for cl in Clauses]
    for (gate, gdef) in RepTable.items():
        #newdef = subst_lits_in_tuple(gdef, MonoRepSubst)
        newdef = tuple([MonoRepSubst.get(x, x) for x in gdef])
        if newdef != gdef:
            RepTable[gate] = newdef


    ##################################################################
    # XOR/ITE 

    cycle = find_cycle(RepTable)
    if cycle:
        return handle_cycle(cycle)


    DelClauses = []

    ite_quarter_def_tups = []
    ite_quarter_def_clauses = PreferablyOrderedDict()
    
    def add_ite_quarter_def(glit, sel, x, cl):
        if abs(glit) in RepTable or not elig[abs(glit)]:
            return
        ite_quarter_def_tups.append((glit, sel, x))
        ite_quarter_def_clauses.setdefault((glit, sel, x), []).append(cl)
    
    if (glo.FindMuxMeth & 6):
        for CurCl in Clauses:
            if len(CurCl) != 3:
                continue
            (L1, L2, L3) = CurCl
            add_ite_quarter_def(L1, L2, L3, CurCl)
            add_ite_quarter_def(L1, L3, L2, CurCl)
            add_ite_quarter_def(L2, L1, L3, CurCl)
            add_ite_quarter_def(L2, L3, L1, CurCl)
            add_ite_quarter_def(L3, L1, L2, CurCl)
            add_ite_quarter_def(L3, L2, L1, CurCl)
    
    ite_quarter_def_tups = list(unique(ite_quarter_def_tups))
    ite_quarter_def_set = set(ite_quarter_def_tups)
    ite_full_defs = PreferablyOrderedDict()
    
    ite_quarter_defs = PreferablyOrderedDict()
    for (glit, sel, x) in ite_quarter_def_tups:
        if ((-glit, sel, -x) not in ite_quarter_def_set):
            continue
        if (glit < 0):
            continue
        ite_quarter_defs.setdefault(glit, PreferablyOrderedDict())
        ite_quarter_defs[glit].setdefault(sel, [])
        ite_quarter_defs[glit][sel].append((x, 
            ite_quarter_def_clauses[(glit, sel, x)] + ite_quarter_def_clauses[(-glit, sel, -x)]))
    
    def xor_equiv_class(gdef):
        assert(gdef[0] == 'ite')
        [sel, tbra, fbra] = gdef[1:]
        if tbra != -fbra:
            return gdef
        xor_args = [sel, fbra]
        if abs(xor_args[1]) < abs(xor_args[0]):
            xor_args = [xor_args[1], xor_args[0]]
        if xor_args[0] < 0:
            xor_args = [-xor_args[0], -xor_args[1]]
        return ('ite', xor_args[0], -xor_args[1], xor_args[1])
            

    ite_full_def_clauses = PreferablyOrderedDict()
    for gvar in ite_quarter_defs.keys():
        if num_clauses_with_var[gvar] == 4:
            # This is variable is not a good candidate for a gate variable,
            # because there are only enough clauses to define it, so there
            # wouldn't be any uses of the gate.
            continue
        for sel in ite_quarter_defs[gvar].keys():
            if sel < 0:
                continue
            if -sel not in ite_quarter_defs[gvar]:
                continue
            if len(ite_quarter_defs[gvar][sel]) != 1 or len(ite_quarter_defs[gvar][-sel]) != 1:
                #sys.stderr.write("Skipped %r due to multiple branches\n" % ((gvar, sel),))
                continue
            # v <==> (s ? t : f) expands to
            # ((s & t) => v) & ((s & ~t) => ~v) & ((~s & f) => v) & ((~s & ~f) => ~v)
            # which expands to
            # (~s | ~t | v)  &  (~s | t | ~v)  &  (s | ~f | v)  &  (s | f | ~v) 
            (neg_tbra, tbra_clauses) = ite_quarter_defs[gvar][-sel][0]
            (neg_fbra, fbra_clauses) = ite_quarter_defs[gvar][sel][0]
            if any(gvar in xsub[abs(ch)] for ch in [sel, neg_tbra, neg_fbra]):
                continue
            ite_full_defs.setdefault(gvar, [])
            gdef = ('ite', sel, -neg_tbra, -neg_fbra)
            ite_full_defs[gvar].append(gdef)
            def_clauses = tbra_clauses + fbra_clauses
            #for cl in def_clauses:
            #    assert(cl in Clauses)
            ite_full_def_clauses[(gvar, gdef)] = def_clauses
        if gvar not in ite_full_defs:
            continue
        ite_full_defs[gvar] = list(unique_by_equiv_class(ite_full_defs[gvar], xor_equiv_class))
    
    class DefEdge(object):
        def __init__(self, gate_var, gdef, confirmed=False):
            self.gate_var = gate_var
            self.gdef = gdef
            self.child_vars = tuple([abs(x) for x in gdef[1:]])
            self.confirmed = confirmed
    
        def __repr__(self):
            return "DefEdge(%r, %r, %r)" % (self.gate_var, self.gdef, self.confirmed)
    
    class LitNode(object):
        def __init__(self, var, parent_edges=None, child_edges=None):
            self.var = var
            if parent_edges is None: parent_edges = []
            if child_edges is None: child_edges = []
            self.parent_edges = parent_edges
            self.child_edges = child_edges
    
        def __repr__(self):
            return "LitNode(%r, parent_edges=%r,\n child_edges=%r)" % (self.var, self.parent_edges, self.child_edges)
    
    pending_edges = PreferablyOrderedDict()
    
    var_to_node = {}
    for i in UsedVars:
        var_to_node[i] = LitNode(i)

    for i in RepTable:
        if i not in var_to_node:
            var_to_node[i] = LitNode(i)

    for (gvar, ite_def_list) in ite_full_defs.items():
        for ite_def in ite_def_list:
            edge = DefEdge(gvar, ite_def)
            var_to_node[gvar].child_edges.append(edge)
            pending_edges[edge] = True
    
    for i in RepTable:
        #for e in var_to_node[i].child_edges:
        #    pending_edges.pop(e, None)
        var_to_node[i].child_edges = [DefEdge(i, RepTable[i], confirmed=True)]
    
    for (var, node) in var_to_node.items():
        for edge in node.child_edges:
            for child_var in edge.child_vars:
                if child_var not in var_to_node:
                    var_to_node[child_var] = LitNode(child_var)
                if edge not in var_to_node[child_var].parent_edges:
                    var_to_node[child_var].parent_edges.append(edge)
    
    top_var_cache = PreferablyOrderedDict()
    bot_var_cache = PreferablyOrderedDict()
    def is_top_var(var): 
        if var in top_var_cache:
            return top_var_cache[var]
        node = var_to_node[var]
        ret = all(e.confirmed and is_top_var(e.gate_var) for e in node.parent_edges)
        top_var_cache[var] = ret
        return ret
    def is_bot_var(var):
        if var in bot_var_cache:
            return bot_var_cache[var]
        node = var_to_node[var]
        ret = all(e.confirmed and all(is_bot_var(ch) for ch in e.child_vars) for e in node.child_edges)
        bot_var_cache[var] = ret
        return ret
    
    
    reached_fixpt = False
    confirmed_edges = []
    paused_edges = []
    chosen_edges = []
    round_check_children = True

    def kill_edge(edge):
        var_to_node[edge.gate_var].child_edges = (
            [ce for ce in var_to_node[edge.gate_var].child_edges if ce != edge])
        for ch in edge.child_vars:
            var_to_node[ch].parent_edges = (
                [pe for pe in var_to_node[ch].parent_edges if pe != edge])
        

    t_round = 0
    while not(reached_fixpt):
        if not(glo.FindMuxMeth & 6):
            break
        reached_fixpt = True
        if (PrStage): 
            sys.stderr.write("ITE/XOR round %3i (found %4i), %.4f sec, %s\n" % 
                (t_round, len(confirmed_edges), elapsed_time(), "T" if round_check_children else 'F'))
        t_round += 1
        if t_round >= 10:
            round_check_children = False
        t1 = timer()
        bot_var_cache.clear()
        top_var_cache.clear()
        bot_ready = [edge for edge in pending_edges if all(is_bot_var(ch) for ch in edge.child_vars)]
        top_ready = [edge for edge in pending_edges if is_top_var(edge.gate_var)]
        #sys.stderr.write("Top/bot calc: %.4f\n" % (timer() - t1))
        ready_edges = list(unique(list(bot_ready) + list(top_ready)))
        ready_edge_set = set(ready_edges)
        #for edge in chosen_edges:
        #    if not(edge in ready_edge_set):
        #        stderr.write("Missed %r!\n" % (edge,))
        chosen_edges = []

        for edge in ready_edges:
            gvar_node = var_to_node[edge.gate_var]
            gvar_edges = gvar_node.child_edges
            if edge not in gvar_edges:
                assert(len(gvar_edges)==1 and gvar_edges[0].confirmed == True)
                continue
            if len(set(gvar_edges) & ready_edge_set) > 1:
                continue
            edge.confirmed = True
            gvar_node.child_edges = [edge]
            del pending_edges[edge]
            confirmed_edges.append(edge)
            reached_fixpt = False


        def vars_in_edge_sorted(edge):
            return unique_conseq_list(sorted([edge.gate_var] + list(edge.child_vars)))

        if True:
            top_var_cache.clear()
            def can_be_made_top(var):
                par_edges = var_to_node[var].parent_edges
                return (
                    any(   par.confirmed and is_top_var(par.gate_var) for par in par_edges) and
                    all(not par.confirmed or is_top_var(par.gate_var) for par in par_edges) and
                    any(not par.confirmed                             for par in par_edges))
            for e in list(pending_edges):
                if len(var_to_node[e.gate_var].child_edges) != 1:
                    continue
                if not can_be_made_top(e.gate_var):
                    continue
                if round_check_children and all(not can_be_made_top(ch) for ch in e.child_vars):
                    continue
                cancel = False
                for par in list(var_to_node[e.gate_var].parent_edges):
                    if par.confirmed:
                        continue
                    if vars_in_edge_sorted(par) == vars_in_edge_sorted(e):
                        continue
                    cancel = True
                if cancel:
                    continue
                for par in list(var_to_node[e.gate_var].parent_edges):
                    if par.confirmed:
                        continue
                    kill_edge(par)
                    if pending_edges.pop(par, None):
                        paused_edges.append(par)
                chosen_edges.append(e)
                reached_fixpt = False
                    
        if reached_fixpt and round_check_children == True:
            round_check_children = False
            reached_fixpt = False


    glo.num_xor_found = 0
    glo.num_ite_found = 0
    ite_xor_gate_vars = []

    def add_ite_gate(glit, sel, PosA, PosB):
        vp1 = prob.NewGvar()
        vp2 = prob.NewGvar()
        RepTable[vp1] = tuple(['and', sel, PosA])
        RepTable[vp2] = tuple(['and', -sel, PosB])
        if (abs(PosA) == abs(PosB)):
            glo.num_xor_found += 1
        else:
            glo.num_ite_found += 1
        ite_xor_gate_vars.append(abs(glit))
        if glit > 0:
            RepTable[glit] = tuple(['or', vp1, vp2])
        else:
            RepTable[-glit] = NegateExpr(tuple(['or', vp1, vp2]))

    if (glo.FindMuxMeth & 2):
        for edge in confirmed_edges:
            assert(edge.gdef[0] == 'ite')
            (sel, PosA, PosB) = edge.gdef[1:]
            add_ite_gate(edge.gate_var, sel, PosA, PosB)
            DelClauses += ite_full_def_clauses[(edge.gate_var, edge.gdef)]


    DelClauses = set(DelClauses)
    Clauses = [clause for clause in Clauses if (clause not in DelClauses)]
    DelClauses = []

    ite_xsub_cache = {}
    def calc_ite_xsub(gvar):
        assert(gvar > 0)
        ret = ite_xsub_cache.get(gvar, None)
        if not(ret is None): return ret
        if gvar not in RepTable:
            ret = ()
        else:
            children = [abs(ch) for ch in RepTable[gvar][1:]]
            ret = []
            for ch in children:
                if ch in RepTable:
                    ret.append(calc_ite_xsub(ch))
                elif elig[ch]:
                    ret.append([ch])
            ret = tuple(unique_conseq(iter(sorted(flatten(ret)))))
        ite_xsub_cache[gvar] = ret
        return ret

    xsub_start_time = timer()
    for gvar in RepTable:
        if glo.FindMuxMeth == 0:
            break
        xsub[gvar] = set(calc_ite_xsub(gvar))
    xsub_end_time = timer()
    ite_xsub_cache = None
    #stderr.write("Time for xsub: %.4f\n" % (xsub_end_time - xsub_start_time,))
    

        
    ################################################################################

    def TryMux(WorkCl, glit, sel):
        # A gate defn of the form (glit = (sel ? a : b)) is encoded by the
        # following four terms:
        # ((sel  & glit  & a)  |
        #  (sel  & ~glit & ~a) | 
        #  (~sel & glit  & b)  | 
        #  (~sel & ~glit & ~b))
        # 
        if (abs(glit) == abs(sel)): return None
        PosA = NegA = PosB = NegB = 0
        def GetIn(cl):
            return (cl - set([glit, -glit, sel, -sel])).pop()
        for cl in WorkCl:
            if    (-sel in cl) and  (glit in cl): NegA = GetIn(cl)
            elif  (-sel in cl) and (-glit in cl): PosA = GetIn(cl)
            elif   (sel in cl) and  (glit in cl): NegB = GetIn(cl)
            elif   (sel in cl) and (-glit in cl): PosB = GetIn(cl)
            else: return None
        if (0 in [PosA, PosB, NegA, NegB]): return None
        if (PosA != -NegA): return None
        if (PosB != -NegB): return None
        if len(set(abs(x) for x in [glit,sel,PosA,PosB])) < 3:
            return None
        return (PosA, PosB)
        
    def LookForMux(WorkCl, CurCl):
        CurCl = [abs(x) for x in CurCl]
        for glit in CurCl:
            if (glit in RepTable): continue
            if (not elig[abs(glit)]): continue
            #if (glit in NotMonoElig): continue
            PosSel = tuple(set(CurCl) - set([glit]))
            for j in [0,1]:
                test = TryMux(WorkCl, glit, PosSel[j])
                if test == None:
                    continue
                if StrictOrder and any((elig[abs(x)]) for x in [PosSel[j], test[0], test[1]]):
                    continue
                if (test != None): return (glit, PosSel[j], test[0], test[1])
        return None

    def IsDesc(child, par, hit):
        if (par in hit): return False
        hit.add(par)
        par = abs(par)
        if (par not in RepTable): return False
        for x in RepTable[par][1:]:
            if abs(x)==abs(child): return True
            if IsDesc(child, x, hit): return True
        return False
        

    if ((glo.FindMuxMeth & 1) and not glo.NoRevEngr): 
        for StrictOrder in [1, 0]:
            flag = False
            for (ixCl, CurCl) in enumerate(Clauses):
                if (ixCl < 3): continue
                WorkCl = [set(Clauses[ixCl-j]) for j in [0,1,2,3]]
                if [len(x) for x in WorkCl] != [3,3,3,3]: continue
                ret = LookForMux(WorkCl, CurCl)
                if ret != None:
                    (glit, sel, PosA, PosB) = ret
                    #dhit = set()
                    #if IsDesc(glit, sel, dhit) or IsDesc(glit, PosA, dhit) or IsDesc(glit, PosB, dhit):
                    #    continue
                    if any(abs(glit) in xsub[abs(sub)] for sub in [sel, PosA, PosB]):
                        continue
                    flag = True
                    add_ite_gate(glit, sel, PosA, PosB)
                    xsub[abs(glit)] = calc_extra_xsub([sel, PosA, PosB])
                    DelClauses += [(Clauses[ixCl-j]) for j in [0,1,2,3]]
                
    #print("Missing ite_full_defs: " + str([gvar for gvar in ite_xor_gate_vars if all(not e.confirmed for e in var_to_node[gvar].child_edges)]))
    DelClauses = set(DelClauses)

    Clauses = [clause for clause in Clauses if (clause not in DelClauses)]
    
    if (glo.FindMuxMeth & 4):
        stderr.write("ITE/XOR: len(confirmed_edges) = %i\n" % (len(confirmed_edges),))
    if (PrStage): sys.stderr.write("Done finding ITE/XOR gate variables (%i ITE, %i XOR), %.4f sec\n" % 
        (glo.num_ite_found, glo.num_xor_found, elapsed_time()))


    # End XOR/ITE 
    ################################################################################

    ################################################################################
    # Begin Plaisted-Greenbaum
    
    pg_round = 0
    pg_flag = glo.UsePlaistedGreenbaum and not(glo.LtdRevEngr)
    NumGatesPrePG = len(RepTable)
    allow_loose_pg = False
    
    OldBinaryClausesWithLit = None
    OldNonbinaryClausesWithLit = None
    
    while pg_flag and pg_round < 200:
        
        pg_round += 1
        if pg_round >= 2:
            allow_loose_pg = True
        BinaryClausesWithLit = {}
        NonbinaryClausesWithLit = {}
        halfdefs = {}

        for cl in Clauses:
            if len(cl) == 1:
                continue
            ClausesWithLit = (BinaryClausesWithLit if (len(cl) == 2) else NonbinaryClausesWithLit)
            for lit in cl:
                if abs(lit) in RepTable:
                    continue
                if not elig[abs(lit)]:
                    continue
                if any((abs(lit) in xsub.get(abs(sub), ())) for sub in cl):
                    continue
                ClausesWithLit.setdefault(lit, [])
                ClausesWithLit[lit].append(cl)
        
        def add_halfdef(glit, gdef, def_clauses):
            if glit < 0:
                glit = -glit
                gdef = ['not', gdef]
            halfdefs.setdefault(glit, [])
            halfdefs[glit].append([gdef, def_clauses])
            
        for lit in [-x for x in UsedVars] + UsedVars:
            def other_lit(cl, lit):
                assert(len(cl) == 2)
                if cl[0] == lit:
                    return cl[1]
                else:
                    assert(cl[1] == lit)
                    return cl[0]
            NumNonbinary = len(NonbinaryClausesWithLit.get(lit, ()))
            NumBinary = len(BinaryClausesWithLit.get(lit, ()))
            if NumBinary + NumNonbinary != num_clauses_with_lit[lit]:
                continue
            if NumNonbinary == 0:
                if NumBinary >= 2:
                    # Example: 
                    # clause 1: or(lit, 5)  # -5 => lit
                    # clause 2: or(lit, 7)  # -7 => lit
                    # Then guess is: lit = or(-5, -7)
                    # So newgate = ite(or(-5, -6), True, lit) 
                    #            = or(-5, -6, lit)
                    #            = or(lit, -5, -6)
                    gdef = ['or'] + [('protected', lit)] + [-other_lit(cl, lit) for cl in BinaryClausesWithLit[lit]]
                    add_halfdef(lit, gdef, BinaryClausesWithLit[lit])
            elif NumNonbinary == 1:
                if len(BinaryClausesWithLit.get(lit, ())) == 0:
                    # Example clause: or(lit, 6, 8)  # and(-6, -8) => lit
                    # Then guess is: lit = and(-6, -8)
                    # So newgate = ite(and(-6, -8), True, lit) 
                    #            =  or(and(-6, -8), lit)
                    #            =  or(lit, and(-6, -8))
                    cl = NonbinaryClausesWithLit[lit][0]
                    gdef = ['or'] + [('protected', lit)] + [['and'] + [-x for x in cl if x != lit]]
                    add_halfdef(lit, gdef, [cl])
                
        def negate_fmla(fmla):
            if type(fmla) is int:
                return -fmla
            if fmla[0] == "and":  return ['or'] +  [negate_fmla(x) for x in fmla[1:]]
            elif fmla[0] == "or": return ['and'] + [negate_fmla(x) for x in fmla[1:]]
            elif fmla[0] == "protected": 
                assert(len(fmla) == 2)
                return ('protected', negate_fmla(fmla[1]))
            else: die("Bad fmla")

        DelClauses = set()

        def is_elig(x):
            return abs(x) <= NumVars and elig[abs(x)] and not (abs(x) in RepTable)
        
        def NumClausesWithVar(v):
            return (
                len(NonbinaryClausesWithLit.get(v, ())) + 
                len(NonbinaryClausesWithLit.get(-v, ())) + 
                len(   BinaryClausesWithLit.get(v, ())) + 
                len(   BinaryClausesWithLit.get(-v, ())))
        
        subst = {}
        subd_var_list = []
        for var in range(0, NumVars + 1):
            good_def = None
            num_defs = len(halfdefs.get(var, ()))
            if num_defs == 1:
                good_def = halfdefs[var][0]
                [gdef, def_clauses] = good_def
                lits_in_def = [x for x in flatten(def_clauses) if abs(x) != var]
                assert(is_elig(var))
                #if any(is_elig(x) for x in lits_in_def) and NumClausesWithVar(var) > 1:
                #if any(is_elig(x) and len(NonbinaryClausesWithLit.get(x, ())) <= 2 for x in lits_in_def):
                if not(allow_loose_pg) and any(is_elig(x) for x in lits_in_def):
                    good_def = None
                #print [xsub[abs(sub)] for sub in lits_in_def]
                if any(var in xsub[abs(sub)] for sub in lits_in_def):
                    good_def = None
                if good_def:
                    assert(len(set(def_clauses) & DelClauses) == 0)
            elif num_defs > 1:
                for cur_def in halfdefs[var]:
                    [gdef, def_clauses] = cur_def
                    lits_in_def = [x for x in flatten(def_clauses) if abs(x) != var]
                    #if all(not(is_elig(x)) or len(NonbinaryClausesWithLit.get(x, ())) > 2 for x in lits_in_def):
                    if all(not(is_elig(x)) for x in lits_in_def) and not any(var in xsub[abs(sub)] for sub in lits_in_def):
                        good_def = cur_def
                        break

            if good_def != None:
                #stderr.write("%i: %r\n" % (var, good_def))
                [gdef, def_clauses] = good_def
                lits_in_def = [x for x in flatten(def_clauses) if abs(x) != var]
                for sublit in lits_in_def:
                    xsub[abs(sublit)] |= calc_extra_xsub(xsub[abs(sublit)])
                new_gate_xsub = calc_extra_xsub(lits_in_def)
                if var in new_gate_xsub:
                    stderr.write("Just missed on a cycle on %i (file %s).\n" % (var, glo.filename))
                    continue
                subd_var_list.append(var)
                if (gdef[0] == 'not'):
                    assert(len(gdef) == 2)
                    gdef = negate_fmla(gdef[1])
                if not all((type(x) is int or x[0]=='protected') for x in gdef[1:]):
                    assert(len(gdef) == 3)
                    assert(type(gdef[1]) is int or gdef[1][0] == 'protected')
                    subexpr = gdef[2]
                    assert(subexpr[0] in ['and','or'])
                    assert(all((type(x) is int) for x in subexpr[1:]))
                    SubGate = prob.NewGvar()
                    RepTable[SubGate] = tuple(subexpr)
                    xsub[SubGate] = calc_extra_xsub(subexpr[1:])
                    #stderr.write("RepTable[%s] = %r  # SubGate\n" % (SubGate, subexpr))  #DEBUG
                    gdef[2] = SubGate
                NewGate = prob.NewGvar()
                RepTable[NewGate] = tuple(gdef)
                #stderr.write("RepTable[%s] = %r\n" % (NewGate, gdef))  #DEBUG
                subst[var] = NewGate
                subst[-var] = -NewGate
                subst[('protected', var)] = var
                subst[('protected', -var)] = -var
                xsub[var] = new_gate_xsub
                xsub[NewGate] = new_gate_xsub
                for cl in def_clauses:
                    DelClauses.add(cl)

        pg_flag = (len(DelClauses) != 0)
        if not(pg_flag) and not(allow_loose_pg):
            pg_flag = True
            allow_loose_pg = True
        old_cl_count = len(Clauses)
        OldClauses = Clauses
        Clauses = [clause for clause in Clauses if (clause not in DelClauses)]
        new_cl_count = len(Clauses)
        if (old_cl_count - new_cl_count != len(DelClauses)):
            die("File %s: DelClauses is wonky!" % (glo.filename,))
        OldClauses = None
        
        if elapsed_time() > 600 and pg_round >= 2:
            pg_flag = False

        if PrStage and (pg_round <= 3 or round(sqrt(pg_round))**2 == pg_round or not pg_flag):
            sys.stderr.write("Plaisted-Greenbaum round %2i: Found %4i gates" % (pg_round, len(subd_var_list)))
            if len(subd_var_list) > 0:
                sys.stderr.write(", reduced clauses from %5i to %5i\n" % (old_cl_count, new_cl_count))
            else:
                sys.stderr.write(", number of clauses is %5i\n" % (new_cl_count,))

        #Clauses = [subst_lits_in_tuple(cl, subst) for cl in Clauses]
        Clauses = [tuple([subst.get(x, x) for x in cl]) for cl in Clauses]
        for (gate, gdef) in RepTable.iteritems():
            #newdef = subst_lits_in_tuple(gdef, subst)
            newdef = tuple([subst.get(x, x) for x in gdef])
            if newdef != gdef:
                RepTable[gate] = newdef


    if (PrStage): sys.stderr.write("Done finding Plaisted-Greenbaum gate variables (%i), %.4f sec\n" % 
        (len(RepTable) - NumGatesPrePG, elapsed_time()))

    
    # End of Plaisted-Greenbaum
    ################################################################################

    NewClauses = list(reversed([tuple(x for x in clause) for clause in Clauses]))

    if (PrStage):
        sys.stderr.write(
            "Clauses reduced clauses from %i to %i\n" % (NumOrigClauses, len(Clauses)))

    def ConvRepPair(key, val):
        NewKey = key
        NewVal = tuple(val)
        if NewKey < 0:
            NewKey = -NewKey
            NewVal = NegateExpr(NewVal)
        return (NewKey, NewVal)
        
    RepTable = dict(ConvRepPair(key,val) for (key,val) in RepTable.items())


    if (glo.JustFindGates): 
        sys.stderr.write("Total gate variables found: %i\n" % len(RepTable))
        sys.exit(0)

    ##################################################################

    # Convert the remaining top-level conjuncts.
    OutGlit = None
    if len(NewClauses) == 1 and len(NewClauses[0]) == 1 and abs(NewClauses[0][0]) in RepTable:
        OutGlit = NewClauses[0][0]
    else:
        OutGlit = []
        for CurCl in NewClauses:
            # (v <==> (x1 | x2 | x3)) expands to
            # (~v | x1 | x2 | x3)   &   (~x1 | v)  &  (~x2 | v)  &  (~x3 | v)
            if len(CurCl) == 1:
                OutGlit.append(CurCl[0])
                continue
            NewVar = prob.NewGvar()
            RepTable[NewVar] = tuple(['or'] + list(CurCl))
            OutGlit.append(NewVar)
        if len(OutGlit) == 1 and abs(OutGlit[0]) in RepTable:
            OutGlit = OutGlit[0]
        else:
            # (v <==> (x1 & x2 & x3)) expands to
            # (~x1 | ~x2 | ~x3 | v)  &  (~v | x1)  &  (~v | x2)  &  (~v | x3)
            if len(OutGlit) > (glo.MaxGateSize)**1.5:
                NewOutGlit = []
                OutGlitLen = len(OutGlit)
                PartSize = sqrt(OutGlitLen) * 1.00001
                glo.MaxGateSize = int(PartSize + 1)
                SubGates = [[]]
                for (ixGlit, CurOutGlit) in enumerate(OutGlit):
                    if (len(SubGates) < ixGlit / PartSize):
                        SubGates.append([])
                    SubGates[-1].append(CurOutGlit)
                #print SubGates
                OutGlit = []
                for CurSubGate in SubGates:
                    NewVar = prob.NewGvar()
                    RepTable[NewVar] = tuple(['and'] + CurSubGate)
                    OutGlit.append(NewVar)

            NewVar = prob.NewGvar()
            RepTable[NewVar] = tuple(['and'] + OutGlit)
            OutGlit = NewVar

    if OutGlit < 0:
        RepTable[-OutGlit] = NegateExpr(RepTable[-OutGlit])
        OutGlit = -OutGlit

        
    prob.QuantPfx = {}
    prob.QuantPfx[OutGlit] = QuantBlks
    prob.OutGlit = OutGlit
    prob.RepTable = RepTable
    prob.NumElim = pNumElim[0]
    prob.OrigNames = glo.OrigNames
    cycle = find_cycle(RepTable)
    if cycle:
        return handle_cycle(cycle)
        
    return prob


################################################################################


def find_cycle(RepTable):
    # Based on http://neopythonic.blogspot.com/2009/01/detecting-cycles-in-directed-graph.html
    todo = set(RepTable.keys())
    while todo:
        node = todo.pop()
        stack = [node]
        while stack:
            top = stack[-1]
            for node in RepTable[top][1:]:
                node = abs(node)
                if node in stack:
                    return stack[stack.index(node):]
                if node in todo:
                    stack.append(node)
                    todo.remove(node)
                    break
            else:
                node = stack.pop()
    return None


def FindGateOccur(p):
    p.NumGateOccur = array('B', [0] * (p.LastGateVar+1))
    for expr in p.RepTable.values():
        for lit in expr[1:]:
            if not p.IsGateLit(lit): continue
            p.NumGateOccur[abs(lit)] = min(2, p.NumGateOccur[abs(lit)]+1)

class UniqPriQueue(object):
    def __init__(self, items, keyf):
        self.s = set(items)
        self.h = list((keyf(x), x) for x in items)
        self.keyf = keyf
        heapq.heapify(self.h)

    def pop(self):
        (k, ret) = heapq.heappop(self.h)
        self.s.remove(ret)
        return ret

    def push(self, item):
        if item in self.s: 
            return
        heapq.heappush(self.h, (self.keyf(item), item))
        self.s.add(item)

    def peek(self):
        return self.h[0][1]

    def __len__(self):
        return len(self.h)


def flatten(LL):
    return [item for sublist in LL for item in sublist]

def has_var_as_descendent(p, gate, targs, cache):
    # Precond: gate and targ are both positive.
    #assert(gate > 0)
    #assert(all(x > 0 for x in targs))
    ret = cache.get(gate, None)
    if ret is not None:
        return ret
    if gate in targs:
        ret = True
        cache[gate] = ret
        return ret
    expr = p.RepTable.get(gate, None)
    if expr is None:
        ret = False
        cache[gate] = ret
        return ret
    else:
        ret = False
        for sublit in expr[1:]:
            subvar = abs(sublit)
            if has_var_as_descendent(p, subvar, targs, cache):
                ret = True
                break
        cache[gate] = ret
        return ret
            


def FindParents(p):
    p.VarPars = {}
    for (RepVar, expr) in p.RepTable.items():
        for lit in expr[1:]:
            p.VarPars.setdefault(abs(lit), [])
            p.VarPars[abs(lit)].append(RepVar)

    return

    def occurrence_polarity(g):
        if g == p.OutGlit:
            return 1
        if len(p.VarPars[g]) != 1:
            return 0
        par = p.VarPars[g][0]
        if g in p.RepTable[par]:
            return occurrence_polarity(par)
        elif -g in p.RepTable[par]:
            return -occurrence_polarity(par)
        else:
            assert(False)


    def get_common_par(anc, cur_nonelig):
        if len(anc.h) == 1 and (anc.peek() not in cur_nonelig):
            #return anc.peek()
            pol = occurrence_polarity(anc.peek())  # 1 (pos), -1 (neg), or 0 (both)
            if pol == 1: return anc.peek()
            if pol == -1: return -anc.peek()
        if p.OutGlit in anc.s:
            return None
        if len(p.VarPars[anc.peek()]) != 1:
            return None
        common_par = p.VarPars[anc.peek()][0]
        if len(p.RepTable[common_par][1:]) == len(anc.h):
            return None
        for y in anc.s:
            if p.VarPars[y] != [common_par]:
                return None
        return common_par
    p.Lca = {}
    nonelig = set()
    assert(p.QuantPfx.keys() == [p.OutGlit])
    qblocks = p.QuantPfx[p.OutGlit]
    cur_nonelig = set([])
    for qb in reversed(qblocks):
        next_nonelig = set(cur_nonelig)
        for var in qb.qvars:
            keyf = (lambda v: (1<<30) if v == p.OutGlit else max(p.VarPars[v]))
            anc = UniqPriQueue(p.VarPars[var], keyf)
            while not get_common_par(anc, cur_nonelig):
                x = anc.pop()
                next_nonelig.add(x)
                for aa in p.VarPars[x]:
                    assert(aa % 2 == 0)
                    anc.push(aa)
            if len(anc.h) > 1:
                for (k, x) in anc.h:
                    next_nonelig.add(x)
            p.Lca[var] = sorted(list(anc.s))
        cur_nonelig = next_nonelig
    #for (qnum, qb) in enumerate(qblocks):
    #    print "### ", qnum+1, ": ", sorted([(p.Lca[v], v) for v in qb.qvars])
    for v in sorted(p.VarsByLev[1]):
        print ("%i:" % v), p.Lca[v]


def OccursOnce(p, glit):
    gvar = abs(glit)
    if (gvar == abs(p.OutGlit)): return True
    pars = p.VarPars[gvar]
    if len(pars) != 1: return False
    return OccursOnce(p, pars[0])

def NegateExpr(expr):
    if (expr == "true"): return "false"
    if (expr == "false"): return "true"
    expr = list(expr)
    expr[0] = {'and':'or', 'or':'and'}[expr[0]]
    def negate_lit(lit):
        if type(lit) is int:
            return -lit
        else:
            assert(type(lit) is tuple and lit[0] == 'protected')
            return ('protected', -lit[1])
    expr[1:] = [negate_lit(arg) for arg in expr[1:]]
    return tuple(expr)
    
def FindOccurPol(p, CurLit, HitSet):
    if (CurLit in HitSet): return
    p.OccurPol.add(CurLit)
    pol = (-1 if (CurLit < 0) else 1)
    if p.IsGateLit(CurLit):
        for SubLit in p.RepTable[abs(CurLit)][1:]:
            FindOccurPol(p, SubLit * pol, HitSet)
    #else:
    #    assert(CurLit in p.PresLits[p.OutGlit])
    HitSet.add(CurLit)
    

def CombineSimplify(p, ixExpr, HitSet):
    """Convert AND(x,AND(y,z)) to AND(x,y,z), etc."""
    if (ixExpr in HitSet): return
    HitSet.add(ixExpr)
    OldTop = p.RepTable[ixExpr]
    if (OldTop == "true" or OldTop == "false"): return
    #CurPresLits = set()
    HasDual = [0]
    NewArgs = set()
    def add(CurList, x):
        if (type(x)==int and (x in p.TrivLits)):
            x = {True:"true", False:"false"}[p.TrivLits[x]]
        if (x == "true"):
            if OldTop[0] == 'and': return
            assert(OldTop[0] == 'or')
            HasDual[0] = True
            return
        if (x == "false"):
            if OldTop[0] == 'or': return
            assert(OldTop[0] == 'and')
            HasDual[0] = True
            return
        if (-x in CurList): HasDual[0] = x
        CurList.add(x)
    for ixSubExpr in OldTop[1:]:
        if not p.IsGateLit(abs(ixSubExpr)):
            add(NewArgs, ixSubExpr)
            continue
        CombineSimplify(p, abs(ixSubExpr), HitSet)
        if ixSubExpr < 0:
            SubExpr = NegateExpr(p.RepTable[-ixSubExpr])
        else:
            SubExpr = p.RepTable[ixSubExpr]
        if (SubExpr == "true" or SubExpr == "false"):
            add(NewArgs, SubExpr)
            continue
        #if glo.NoMonoSimp:
        #    SubPres = set()
        #else:
        #    SubPres = p.PresLits[abs(ixSubExpr)]
        #    if (p.NumGateOccur[abs(ixSubExpr)] < 2):
        #        p.PresLits[abs(ixSubExpr)] = None
        #    if ixSubExpr < 0:
        #        SubPres = set(-x for x in SubPres)
        #
        #CurPresLits |= SubPres

        if (len(SubExpr[1:]) == 1) and (ixSubExpr not in p.QuantPfx):
            add(NewArgs, SubExpr[1:][0])
            p.PossibleJunk.add(abs(ixSubExpr))
            continue

        
        if (SubExpr[0] == OldTop[0]) and (glo.OptCombine >= p.NumGateOccur[abs(ixSubExpr)]) and (ixSubExpr not in p.QuantPfx) and (-ixSubExpr not in NewArgs) and (len(SubExpr) + len(NewArgs) < glo.MaxGateSize):
            for NewLit in SubExpr[1:]:
                add(NewArgs, NewLit)
            p.PossibleJunk.add(abs(ixSubExpr))
            p.NumCombined += 1
        else:
            add(NewArgs, ixSubExpr)
    if (HasDual[0]):
        #assert(False)
        if (OldTop[0] == 'or'):     p.RepTable[ixExpr] = "true"
        elif (OldTop[0] == 'and'):  p.RepTable[ixExpr] = "false"
        else: assert(False)
        return
    if len(NewArgs)==0:
        if (OldTop[0] == 'or'):     p.RepTable[ixExpr] = "false"
        elif (OldTop[0] == 'and'):  p.RepTable[ixExpr] = "true"
        else: assert(False)
        return
    #if not glo.NoMonoSimp:
    #    CurPresLits |= set([x for x in NewArgs if True or not p.IsGateLit(x)])
    #    assert(ixExpr > 0)
    #    p.PresLits[ixExpr] = CurPresLits
    NewArgs = [OldTop[0]] + sorted(NewArgs, key=abs)
    p.RepTable[ixExpr] = tuple(NewArgs)

def TrimJunk(p):
    """Removes expressions made redundant by CombineSimplify."""
    RefdExprs = set()
    def AddRefd(ixExpr):
        if (ixExpr in RefdExprs):
            return
        RefdExprs.add(ixExpr)
        if (ixExpr not in p.RepTable):
            return
        Expr = p.RepTable[ixExpr]
        for arg in Expr[1:]:
            AddRefd(abs(arg))
    AddRefd(abs(p.OutGlit))
    UselessExprs = []
    Subsumed = set()
    for ixExpr in p.RepTable.keys():
        if (ixExpr not in RefdExprs):
            if (ixExpr not in p.PossibleJunk):
                UselessExprs.append(p.MapNewToOld[ixExpr])
            else:
                Subsumed.add(p.MapNewToOld[ixExpr])
            del p.RepTable[ixExpr]
            if (ixExpr in p.QuantPfx):
                del p.QuantPfx[ixExpr]

    #if len(UselessExprs) > 0:
    #    sys.stderr.write("Note: Input file has useless subformulas, headed " + \
    #            "by the following variables: " + str(UselessExprs) + "\n")
    return (Subsumed, UselessExprs)

def FindVarLevs(p):
    LevByVar = [0] * (p.NumVars + 1)
    VarsByLev = {}
    GateVars = set(p.RepTable.keys())
    hit = set()
    stack = []
    def FindVarLev(var, rec):
        if (rec > glo.RecLim):
            qdie("ERROR: Exceeded recursion limit!\n" + 
                 "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n")
        var = abs(var)
        if (LevByVar[var] != 0): return LevByVar[var]
        if var in hit:
            die("ERROR: Cycle in RepTable!\n") 
        hit.add(var)
        if (var not in GateVars): 
            ret = 1
        else:
            stack.append(var)
            ChildLevs = [FindVarLev(x,rec+1) for x in p.RepTable[var][1:]]
            stack.pop()
            ret = max(ChildLevs)+1
        LevByVar[var] = ret
        VarsByLev.setdefault(ret, set())
        VarsByLev[ret].add(var)
        return ret
    FindVarLev(p.OutGlit, 100)
    for QuantBlks in p.QuantPfx.values():
        for OrigQb in QuantBlks:
            qtype = OrigQb.qtype
            assert(qtype.lower() in ('a', 'e', 'f'))
            QbSet = set(OrigQb.qvars)
            for lit in OrigQb.qvars:
                if LevByVar[lit] != 1:
                    QbSet.remove(lit)
            OrigQb.qvars = list(sorted(QbSet))
    if (PrStage): sys.stderr.write("Done FindVarLevs, %.4f sec\n" % elapsed_time())
    p.VarsByLev = VarsByLev
    p.LevByVar = LevByVar
    return 

def FindQtypes(p):
    p.QtypeFromVar = {}
    for qp in p.QuantPfx.values():
        for qb in qp:
            for var in qb.qvars:
                if (var in p.QtypeFromVar):
                    die("Variable %i is quantified in more than once place!" % var)
                if (glo.input_format == 'cqbf' and var in p.RepTable):
                    die("Variable %i is quantified but also used a gate variable!" % var)
                p.QtypeFromVar[var] = qb.qtype.lower()

def ChkUnquantVars(p):
    QuantVars = set()
    for qp in p.QuantPfx.values():
        for qb in qp:
            QuantVars |= set(qb.qvars)
    unq = set()
    for (lev, lvars) in p.VarsByLev.items():
        unq |= (lvars - QuantVars)
    if len(unq) > 0:
        print "Warning: unquantified variables: ", unq

def RenumExprs(p):
    """Renumber the expressions.  Postcond: Every new expr number will be even."""
    VarsByLev = p.VarsByLev
    InpVars = VarsByLev[1]
    QuantVars = set()
    for qp in p.QuantPfx.values():
        for qb in qp:
            QuantVars |= set(qb.qvars)
    #
    FreeVars = sorted(InpVars - QuantVars)
    if (len(FreeVars) > 0): 
        print "Error: There are unquantified variables in the QBF matrix:"
        print FreeVars
        qdie("")
    #
    map = {}
    increm = glo.VarIncrem
    i = glo.InputVarStart
    p.QtypeFromVar = {}
    for (qgate, blks) in SortedQuantPfx(p):
        for qb in blks:
            assert(qb.qtype.upper() in ['A','E','F'])
            for OldVar in sorted(set(qb.qvars) & InpVars):
                map[OldVar] = i
                i += increm
    p.NumVars = i - increm

    NewRepTbl = {}
    def conv(val):
        if type(val)==str: return val
        assert(type(val)==int)
        if val > 0: return map[val]
        if val < 0: return -map[abs(val)]

    #i = int('1' + '0'*len(str(p.NumVars)))
    #if (p.NumVars >= 100):
    #    i = int( str(int(str(p.NumVars)[0])+1) + '0'*(len(str(p.NumVars))-1) )
    #base = i
    base = ((p.NumVars // 100) + 1) * 100
    p.FirstExpr = base + increm 
    i = p.FirstExpr
    for lev in sorted(VarsByLev.keys()):
        if lev <= 1: continue
        for OldExprIx in (sorted(VarsByLev[lev])):
            map[OldExprIx] = i
            NewRepTbl[i] = tuple([conv(x) for x in p.RepTable[OldExprIx]])
            i += increm
    p.LastGateVar = max(map.values())

    p.OutGlit = conv(p.OutGlit)
    OldQuantPfx = p.QuantPfx
    p.QuantPfx = {}
    for (qgate, blks) in OldQuantPfx.items():
        for qb in blks:
            qb.qvars = [abs(conv(x)) for x in qb.qvars]
        p.QuantPfx[conv(qgate)] = blks
    p.MapNewToOld = dict([(val, key) for (key, val) in map.items()])
    LevByOldVar = p.LevByVar
    p.LevByVar = {}
    p.VarsByLev = {}
    for (key, expr) in p.RepTable.items():
        if (LevByOldVar[key] == 0): continue
        NewExpr = NewRepTbl[map[key]]
        assert(NewExpr[0] == expr[0])
        for i in range(1, len(expr)):
            old = map[abs(expr[i])]
            if (expr[i] < 0): old = -old
            assert(NewExpr[i] == old)
    for (OldVar, lev) in enumerate(LevByOldVar):
        if OldVar not in map:
            continue
        NewVar = map[OldVar]
        p.LevByVar[NewVar] = lev
        p.VarsByLev.setdefault(lev, set())
        p.VarsByLev[lev].add(NewVar)
    p.OldRepTable = p.RepTable
    p.RepTable = NewRepTbl


def merge_prefixes(pfx_list):
    ret = []
    while not all(pfx == [] for pfx in pfx_list):
        for cur_qtype in ['e', 'a']:
            for pfx in pfx_list:
                while (len(pfx) > 0) and (pfx[0].qtype == cur_qtype):
                    ret.append(pfx[0])
                    pfx[:] = pfx[1:]
    return ret
            


def Prenex(p, gate, mode, hit):
    if gate < 0:
        Prenex(p, abs(gate), mode, hit)
        if len(p.QuantPfx.get(abs(gate), [])) > 0:
            die("ERROR: Prefixed gate %i occurs negatively." % abs(gate))
        return
    if (gate in hit) or (not p.IsGateLit(gate)):
        return
    hit.add(gate)
    p.QuantPfx.setdefault(gate, [])
    child_pfxs = []
    for sub in p.RepTable[gate][1:]:
        Prenex(p, sub, mode, hit)
        if sub in p.QuantPfx:
            child_pfxs.append(p.QuantPfx[sub])
            del p.QuantPfx[sub]
    if mode == 1:
        child_pfxs = flatten(child_pfxs)
    elif mode == 2:
        child_pfxs = merge_prefixes(child_pfxs)
    else:
        die("Invalid prenex mode: %i." % mode)
    p.QuantPfx[gate] += child_pfxs
    if len(p.QuantPfx[gate]) == 0:
        del p.QuantPfx[gate]


def SortedQuantPfx(p):
    return sorted(p.QuantPfx.items(), key=(lambda qp: (-p.LevByVar[qp[0]], -abs(qp[0]))))
                

class TrivLitDict:
    def __init__(self):
        self.d = dict()
    
    def __setitem__(self, k, v):
        assert(v in [True, False])
        assert(type(k)==int)
        if (k < 0):
            k = -k
            v = (not v) 
        self.d[k] = v
    
    def __getitem__(self, k):
        if (k < 0): return (not self.d[-k])
        return self.d[k]
    
    def __contains__(self, k):
        return self.d.__contains__(abs(k))

def FindTrivLits(p):
    p.TrivLits = TrivLitDict()
    TopExpr = p.RepTable[p.OutGlit]
    if (TopExpr[0] == 'and'):
        for lit in TopExpr[1:]:
            if not p.IsGateLit(lit):
                qtype = p.QtypeFromVar[abs(lit)].lower()
                if qtype != 'f':
                    p.TrivLits[lit] = {'e':True, 'a':False}[qtype]



TrivTrueFileText = (
"""
c Trivially true
p cnf 4 1
e 2 4 0
2 4 0
""")

TrivFalseFileText = (
"""
c Trivially false 
p cnf 4 1
a 2 4 0
2 4 0
""")

def GetTrivFalseFile(): return StringIO.StringIO(TrivFalseFileText)
def GetTrivTrueFile() : return StringIO.StringIO(TrivTrueFileText)


class GloOpt:
    pass


def main():
    argv = sys.argv
    if len(argv) < 2 or (argv[1] in ["-h", "--help", "-help"]):
        print "Usage: 'python %s inputfile [-o outfile] [options]'" % (sys.argv[0])
        print "\n".join([
            "Options: ",
            " '-quiet':         Write less output to stderr.",
            " '-no-renum':      Don't renumber the variables.",
            " '-no-rev-engr':   Don't attempt to reverse-engineer the CNF input file.",
            " '-no-pg':         Don't attempt to reverse-engineer Plaisted-Greenbaum.",
            " '-no-simplify':   Don't attempt to perform certain simplifications.",
            " '-count-gates':   Find Tseitin vars, print count, and exit.",
            " '-no-allow-or':   Don't use any OR gates; instead use negated AND gates.",
            " '-prenex [MODE]': Converts the formula to prenex form.  MODE is 1 or 2.",
            " '-keep-varnames:  Print a mapping between old variable numbers and new.",
            " '-no-varnames:    Don't print a mapping between old variable numbers and new.",
            " '-var-num-pfx s': Prepend variable numbers with string s.  Default is 'v'.",
            " '-write-qdimacs': Write a QDIMACS file (requires '-no-allow-or' option).",
            " '-write-qcir':    Write a QCIR-format circuit file.",
            " '-write-gq':      Write a GhostQ-format circuit file.",
            " '-find-ite N':    Method for finding ITE/XOR gates (0=none, 1=old, 2=new).",
            " '-reclim N':      Recursion limit.",
            ])
        sys.exit(1)

    global PrStage
    global glo
    glo = GloOpt()

    glo.VarIncrem = 2

    filename = sys.argv[1]
    glo.filename = filename
    if not os.path.exists(filename) and filename != "-":
        qdie("Input file '" + filename + "' doesn't exist!")
    PrettyName = filename
    OnlyAnd = False
    WriteOldMap = False
    glo.OutputFmt = 'qcir'
    glo.NoRevEngr = False
    glo.UsePlaistedGreenbaum = True
    glo.LtdRevEngr = False
    glo.NoCompanions = False
    glo.PrintCompanionGates = False
    glo.NoMonoRep = False
    glo.MonoClauseLimit = 100000 
    glo.MonoVarLimit = 20000
    glo.NoMonoSimp = False
    glo.JustFindGates = False
    glo.InputVarStart = 2
    glo.OptCombine = 1
    glo.NoRenum = False
    glo.KeepVarNames = False
    glo.VarNumPfx = "v"
    glo.NoSimplify = False
    glo.MinSimplify = False
    glo.NoVarAlias = True
    glo.ConvOr = False
    glo.MaxGateSize = 40000
    glo.RecLim = 5000
    glo.NoPrintPreprocTime = False
    glo.Debug = False
    glo.PrenexMode = False
    glo.OrigNames = {}
    glo.FindMuxMeth = 2
    OutFilename = ""
    outf = sys.stdout
    i=2
    while (i < len(argv)):
        sArg = argv[i]
        if sArg.startswith('--'):
            sArg = sArg[1:]
        has_param = ((i+1 < len(argv)) and not argv[i+1].startswith('-'))
        if (False): pass
        elif (sArg=='-no-conv-or'):     glo.ConvOr = False
        elif (sArg=='-conv-or'):        glo.ConvOr = True
        elif (sArg=='-no-allow-or'):    OnlyAnd = True
        elif (sArg=='-old-map'):        WriteOldMap = True
        elif (sArg=='-write-qdimacs'):  glo.OutputFmt = 'qdimacs'
        elif (sArg=='-write-gq'):       glo.OutputFmt = 'cqbf'
        elif (sArg=='-write-qcir'):     glo.OutputFmt = 'qcir'
        elif (sArg=='-no-rev-engr'):    glo.NoRevEngr = True
        elif (sArg=='-use-pg'):         glo.UsePlaistedGreenbaum = True
        elif (sArg=='-no-pg'):          glo.UsePlaistedGreenbaum = False
        elif (sArg=='-ltd-rev-engr'):   glo.LtdRevEngr = True
        elif (sArg=='-no-mono-rep'):    glo.NoMonoRep = True
        elif (sArg=='-no-mono-simp'):   glo.NoMonoSimp = True
        elif (sArg=='-count-gates'):    glo.JustFindGates = True
        elif (sArg=='-no-companions'):  glo.NoCompanions = True
        elif (sArg=='-print-companions'): glo.PrintCompanionGates = True
        elif (sArg=='-no-renum'):       glo.NoRenum = True
        elif (sArg=='-keep-varnames'):  glo.KeepVarNames = True
        elif (sArg=='-no-varnames'):    glo.KeepVarNames = False 
        elif (sArg=='-no-simplify'):    glo.NoSimplify = True
        elif (sArg=='-min-simplify'):   glo.MinSimplify = True
        elif (sArg=='-incr1'):          glo.VarIncrem = 1
        elif (sArg=='-quiet'):          PrStage = False
        elif (sArg=='-q'):              PrStage = False
        elif (sArg=='-deterministic'):  glo.NoPrintPreprocTime = True
        elif (sArg=='-debug'):          glo.Debug = True
        elif (sArg=='-prenex' and not has_param):
            glo.PrenexMode = 2
        elif (not has_param):  
            qdie("Error: option '%s' doesn't exist or it needs a parameter" % sArg)
        elif (sArg=='-prenex'):
            i+=1
            glo.PrenexMode = int(argv[i])
        elif (sArg=='-var-num-pfx'):
            i+=1
            glo.VarNumPfx = argv[i]
        elif (sArg=='-combine'):
            i+=1
            glo.OptCombine = int(argv[i])
        elif (sArg=='-allow-var-alias'):
            i+=1
            glo.NoVarAlias = not bool(argv[i])
        elif (sArg=='-inp-var-start'):
            i+=1
            glo.InputVarStart = int(argv[i])
        elif (sArg=='-name'):
            i+=1
            PrettyName = argv[i]
        elif (sArg=='-reclim'):
            i+=1
            glo.RecLim = int(argv[i])
        elif (sArg=='-find-ite'):
            i+=1
            glo.FindMuxMeth = int(argv[i])
        elif (sArg=='-o'):
            i+=1
            OutFilename = argv[i]
            if OutFilename.endswith(".qdimacs"):
                glo.OutputFmt = 'qdimacs'
        else: qdie("Error: unrecognized option '%s'" % sArg)
        i += 1

    sys.setrecursionlimit(glo.RecLim)

    if glo.OutputFmt == 'qdimacs':
        OnlyAnd = True
        
    def TransmogrifyProb(p):
        global PrStage
        p.LastGateVar = max(p.RepTable.keys())
        def KillDups():
            if len(p.QuantPfx) > 1:
                return
            again = 1
            while again:
                again = 0
                InvRepTable = dict([(v,k) for (k,v) in sorted(p.RepTable.items())])
                xlat = {}
                for (RepVar, expr) in p.RepTable.items():
                    xlat[RepVar] = InvRepTable[expr]
                for (old, new) in xlat.items():
                    if old != new:
                        again = 1
                        #print "%i = %i : " % (old, new),  p.RepTable[new]
                        del p.RepTable[old]
                for (RepVar, expr) in p.RepTable.items():
                    p.RepTable[RepVar] = tuple([(xlat.get(x,x) if x > 0 else -xlat.get(-x,-x))
                                                for x in expr])
                del xlat

        if glo.PrenexMode:
            Prenex(p, p.OutGlit, glo.PrenexMode, set())
            if (PrStage): sys.stderr.write("Done prenexing, %.4f sec\n" % (elapsed_time()))

        p.Subsumed = set()
        p.MapNewToOld = dict([(x,x) for x in range(0, p.LastGateVar+1)])
        #if not glo.NoRenum: 
        #    RenumExprs(p)
        FindQtypes(p)
        if not glo.MinSimplify:
            FindTrivLits(p)
        else:
            p.TrivLits = TrivLitDict()
        again = True
        # if (not glo.NoSimplify):
        #     FindVarLevs(p)
        #     FindParents(p)
        #     for var in range(1, p.NumVars):

        def ElimRedundant(RepTable):
            ret = {}
            rev = {}
            for key in sorted(RepTable.keys()):  # Sort keys for deterministic behavior
                val = RepTable[key]
                if val in rev:
                    val = ('and', rev[val])
                ret[key] = val
                rev[val] = key
                rev[NegateExpr(val)] = NegateExpr(('and', key))
            return ret
                
        p.PossibleJunk = set()
        (Subsumed, Useless) = TrimJunk(p)
        if Useless and (PrStage): 
            sys.stderr.write("Eliminated %i useless gates.\n" % len(Useless))
        while (again and (not glo.NoSimplify)):
            old_reptable = dict(p.RepTable)
            again = False
            KillDups()
            #p.PresLits = {}  # Lits that are present in the subformula
            StartNumGates = len(p.RepTable)
            p.PossibleJunk = set()
            p.NumCombined = 0
            FindGateOccur(p)
            CombineSimplify(p, p.OutGlit, set())
            if (p.NumCombined > 0): again = True
            if p.RepTable[p.OutGlit] == "true":
                glo.NoRevEngr = True
                glo.NoSimplify = True
                if PrStage:
                    stderr.write("**Problem is trivially %s.**\n" % str(p.RepTable[p.OutGlit]))
                PrStage = False
                return TransmogrifyProb(read_qdimacs_file(LineReader(GetTrivTrueFile())))
            if p.RepTable[p.OutGlit] == "false":
                glo.NoRevEngr = True
                glo.NoSimplify = True
                if PrStage:
                    stderr.write("**Problem is trivially %s.**\n" % str(p.RepTable[p.OutGlit]))
                PrStage = False
                return TransmogrifyProb(read_qdimacs_file(LineReader(GetTrivFalseFile())))
            while (len(p.RepTable[p.OutGlit][1:]) == 1):
                #sys.stderr.write("\n" + "Reached untested code.  Verify it before proceeding.\n")
                #pdb.set_trace()
                NewOut = p.RepTable[p.OutGlit][1:][0]
                if (not p.IsGateLit(NewOut)):
                    TrivFile = {
                        'e': GetTrivTrueFile(),
                        'a': GetTrivFalseFile()}[p.QtypeFromVar[abs(NewOut)]]
                    glo.NoRevEngr = True
                    glo.NoSimplify = True
                    comment = {
                        'e': 'true',
                        'a': 'false'}[p.QtypeFromVar[abs(NewOut)]]
                    if PrStage:
                        stderr.write("**Problem is trivially %s!**\n" % comment)
                    PrStage = False
                    return TransmogrifyProb(read_qdimacs_file(LineReader(TrivFile)))
                if (NewOut < 0):
                    assert(NewOut not in p.QuantPfx)
                    NewOut = -NewOut
                    p.RepTable[NewOut] = NegateExpr(p.RepTable[NewOut])
                p.QuantPfx[NewOut] = p.QuantPfx.setdefault(p.OutGlit, []) + p.QuantPfx.get(NewOut, [])
                #p.PresLits[NewOut] = p.PresLits[p.OutGlit]
                del p.QuantPfx[p.OutGlit]
                del p.RepTable[p.OutGlit]
                p.OutGlit = NewOut
            TrivKeys = [k for (k,v) in p.RepTable.items() if v in ("true", "false")]
            if (p.OutGlit in TrivKeys):
                die("Problem is trivially " + str(p.RepTable[p.OutGlit]))
            TrivKeys = set(TrivKeys)
            for x in TrivKeys:
                del p.RepTable[x]
                if (x in p.QuantPfx):
                    del p.QuantPfx[x]
            for (key, expr) in p.RepTable.items():
                assert(len(set([abs(x) for x in expr[1:]]) & TrivKeys)==0)
            (Subsumed, Useless) = TrimJunk(p)
            p.Subsumed |= Subsumed
            assert(p.OutGlit > 0)
            QuitEarly = (elapsed_time() > 300) or glo.MinSimplify
            MonoSubst = []
            if not QuitEarly:
                p.OccurPol = set()
                FindOccurPol(p, p.OutGlit, set())
                #for lit in range(0, p.NumVars + 1):
                #    if not p.IsGateLit(lit):
                #        assert((lit in p.PresLits[p.OutGlit]) == (lit in p.OccurPol))
                FindTrivLits(p)
                if len(p.TrivLits.d) > 0: again = True
                #TopPresLits = p.PresLits[p.OutGlit]
                TopPresLits = p.OccurPol
                if not glo.NoMonoSimp:
                    MonoSubst = []
                    for lit in range(0, p.NumVars + 1):
                        if p.IsGateLit(lit):
                            continue
                        if (lit in TopPresLits) == (-lit in TopPresLits):
                            continue
                        if   (lit not in TopPresLits): polarity = False
                        elif (-lit not in TopPresLits): polarity = True
                        else: assert(0)
                        if p.QtypeFromVar[lit] == 'f':
                            continue
                        if   p.QtypeFromVar[lit] == 'e': p.TrivLits[lit] = polarity
                        elif p.QtypeFromVar[lit] == 'a': p.TrivLits[lit] = (not polarity)
                        else: assert(0)
                        MonoSubst.append(lit * [-1,1][p.TrivLits[lit]])
                        again = True
                    #print "Monotone: ", MonoSubst
                    if 0:
                        sys.stderr.write("Monotone: ")
                        for x in MonoSubst:
                            sys.stderr.write("%i(%s) " % (x, p.QtypeFromVar[abs(x)].lower()))
                        sys.stderr.write("\n")
            if (PrStage): sys.stderr.write("Done CombineSimplify (%2i combined, %2i gates elim, %2i mono lits elim), %.4f sec\n" % (p.NumCombined, StartNumGates - len(p.RepTable), len(MonoSubst), elapsed_time()))
            del p.NumCombined
            del p.PossibleJunk
            #del p.PresLits
            if QuitEarly:
                stderr.write("Quit simplifying early.\n")
                again = False
        del p.TrivLits

        #if 1:
        #    VarCount = array('l', [0] * (p.NumVars+1))
        #    for (key, expr) in p.RepTable.items():
        #        for lit in expr[1:]:
        #            VarCount[abs(lit)] += 1
        #
        #    NumClauses = len(p.RepTable)
        #    SplitVars = []
        #    for var in irange(1, p.NumVars):
        #        if VarCount[var] > NumClauses / 8:
        #            SplitVars.append(var)
        #            print "Var %5i: %5i occurrences" % (var, VarCount[var])
        #    assert(p.OutGlit > 0)
        #    cache = {}
        #    IndepGates = []
        #    AncGates = []   # Ancestor gates
        #    for gate in p.RepTable[p.OutGlit][1:]:
        #        gate = abs(gate)
        #        if has_var_as_descendent(p, gate, set(SplitVars), cache):
        #            AncGates.append(gate)
        #        else:
        #            IndepGates.append(gate)
        #    #print "Indep: ", IndepGates
        #    IndepBranch = p.NewGvar()
        #    AncBranch = p.NewGvar()
        #    OutOp = p.RepTable[p.OutGlit][0]
        #    p.RepTable[IndepBranch] = tuple([OutOp] + IndepGates)
        #    p.RepTable[AncBranch] = tuple([OutOp] + AncGates)
        #    p.RepTable[p.OutGlit] = tuple([OutOp] + [IndepBranch, AncBranch])


                    
                    


        FindVarLevs(p)
        if not glo.NoRenum: 
            RenumExprs(p)
            if (PrStage): sys.stderr.write("Done renumbering, %.4f sec\n" % elapsed_time())

        Negd = set()
        if OnlyAnd:
            if p.RepTable[p.OutGlit][0] == "or":
                p.OutGlit = -(p.OutGlit)
            for (RepVar, Expr) in p.RepTable.items(): 
                if Expr[0] == 'or': 
                    Negd.add(RepVar)
                    p.MapNewToOld[RepVar] = -p.MapNewToOld[RepVar]

        if glo.ConvOr and len(p.QuantPfx.keys())==1 and (not OnlyAnd):
            FindParents(p)
            for (RepVar, Expr) in sorted(p.RepTable.items(), key=(lambda (k,v): -p.LevByVar[k])): 
                for arg in Expr[1:]:
                    if len(p.VarPars[abs(arg)])==1 and ((arg < 0) ^ (RepVar in Negd)):
                        assert((abs(arg) in p.RepTable) or p.QtypeFromVar[abs(arg)] != 'f')
                        assert(arg not in p.QuantPfx)
                        assert(-arg not in p.QuantPfx)
                        Negd.add(abs(arg))
                        p.MapNewToOld[RepVar] = -p.MapNewToOld[RepVar]

        if Negd != set():
            for qgate in p.QuantPfx.keys():
                if (qgate in Negd):
                    sys.stderr.write("\nERROR: Tried to negate a quantified gate.")
                    if glo.OutputFmt == 'qdimacs':
                        sys.stderr.write("\n(Converting non-prenex to QDIMACS is not implemented.)\n")
                    sys.exit(1)
                    
            for (RepVar, Expr) in p.RepTable.items():
                Expr = list(Expr)
                if RepVar in Negd:
                    if (Expr[0] == "or"): Expr[0] = "and"
                    elif (Expr[0] == "and"): Expr[0] = "or"
                    Expr[1:] = (-x for x in Expr[1:])
                Expr[1:] = ((x if (abs(x) not in Negd) else -x) for x in Expr[1:])
                p.RepTable[RepVar] = tuple(Expr)
        if (PrStage): sys.stderr.write("Done processing, %.4f sec\n" % elapsed_time())
        

        return p

    def CondensePfx(p):
        for (qgate, blks) in p.QuantPfx.items():
            NewBlks = []
            for qb in blks:
                if len(qb.qvars)==0: continue  # no variables quantified
                if len(NewBlks) > 0 and NewBlks[-1].qtype == qb.qtype:
                    NewBlks[-1].qvars += qb.qvars
                else:
                    NewBlks.append(qb)
            p.QuantPfx[qgate] = NewBlks

    ############################################################
    def PrintGhostQ(p, outf):
        NumOrigVars = p.NumOrigVars
        if (not glo.NoRenum):
            NumNewVars = (p.NumVars / glo.VarIncrem)
            factor = NumOrigVars / NumNewVars
            if PrStage:
                sys.stderr.write((
                    "Input variables reduced from %5i to %4i (%ix), NumClauses=%i\n") % \
                    (NumOrigVars, NumNewVars, factor, sum(len(v)+1 for v in p.RepTable.values())))

        outf.write("CktQBF\n")
        outf.write("LastInputVar %i\n" % p.NumVars)
        #outf.write("FirstGateVar %i\n" % p.FirstExpr)
        outf.write("LastGateVar %i\n"  % max(p.RepTable.keys()))
        outf.write("OutputGateLit %i\n"   % p.OutGlit)
        if not glo.NoPrintPreprocTime:
            outf.write("PreprocTimeMilli %i\n"   % int(elapsed_time() * 1000))
        
        if WriteOldMap:
            outf.write('\n' + textwrap.fill(
                "#MapNewToOld (NewVar, OldVar) : (" + 
                ", ".join("(%i,%i)" % pair for pair in sorted(p.MapNewToOld.items()))
                + ")", 80, subsequent_indent='#   ') + "\n\n")
            outf.write('\n' + textwrap.fill(
                "#Subsumed : (" + str(sorted(p.Subsumed)) + ")", 80, subsequent_indent='#   ') + "\n\n")

        if glo.KeepVarNames:
            outf.write('\n')
            for (NewNum, OrigName) in p.get_list_new_to_old():
                outf.write("VarName %3i : %s\n" % (NewNum, OrigName))
        ixQb = 1
        outf.write("\n")

        CondensePfx(p)

        for (qgate, blks) in SortedQuantPfx(p):
            if blks == []:
                continue
            outf.write("<q gate=%i>\n" % qgate)
            for quant in blks:
                if len(quant.qvars)==0: continue  # no variables quantified
                outf.write("#Qb%i\n" % ixQb)
                ixQb += 1
                outf.write("      %s %s\n" % 
                    (quant.qtype.lower(), " ".join([str(lit) for lit in quant.qvars])))
            outf.write("</q>\n")
        outf.write("\n")

        CurLev = 0
        #FindParents(p)
        for (RepVar, Expr) in reversed(sorted(p.RepTable.items())):  #, key=(lambda x: p.MapNewToOld[x])
            NewLev = p.LevByVar[RepVar]
            if (CurLev != NewLev):
                CurLev = NewLev
                if not glo.NoSimplify:
                    outf.write("#NESTING LEVEL L%i\n" % CurLev)
            OccurMsg = ""
            if 0:
                OccOnce = OccursOnce(p, RepVar)
                OccurMsg = (" # occurs once" if OccOnce else " # occurs multiple times")
            outf.write("%i = %s(%s)%s\n" % 
                (RepVar, Expr[0], ", ".join([str(lit) for lit in sorted(Expr[1:], key=abs)]), OccurMsg))
        if (PrStage): sys.stderr.write("Done writing output file, %.4f sec\n" % elapsed_time())

    ############################################################
    def PrintQcir(p, outf):
        NumOrigVars = p.NumOrigVars
        if (not glo.NoRenum):
            NumNewVars = (p.NumVars / glo.VarIncrem)
            factor = NumOrigVars / NumNewVars
            if PrStage:
                sys.stderr.write((
                    "Input variables reduced from %5i to %4i (%ix)\n") % \
                    (NumOrigVars, NumNewVars, factor))

        outf.write("#QCIR-G14\n")
        
        if glo.KeepVarNames:
            outf.write('\n')
            for (NewNum, OrigName) in p.get_list_new_to_old():
                outf.write("#VarName %3i : %s\n" % (NewNum, OrigName))
            outf.write("\n")

        CondensePfx(p)

        qpfx = [(gate, pfx) for (gate, pfx) in SortedQuantPfx(p) if len(pfx) > 0]

        if (len(qpfx) != 1):
            die("Only prenex instances are supported.")
        [(qgate, blks)] = qpfx
        if (qgate != p.OutGlit):
            die("Only prenex instances are supported.")

        for quant in blks:
            if len(quant.qvars)==0: continue  # no variables quantified
            quantifier = {'e':'exists', 'a':'forall', 'f':'free'}[quant.qtype.lower()]
            outf.write("%s(%s)\n" % 
                (quantifier, ", ".join([str(lit) for lit in quant.qvars])))

        outf.write("output(%i)\n" % p.OutGlit)
        outf.write("\n")

        CurLev = 0
        for (RepVar, Expr) in sorted(p.RepTable.items(), key=lambda x: (p.LevByVar[x[0]], x[0])):
            NewLev = p.LevByVar[RepVar]
            if (CurLev != NewLev):
                CurLev = NewLev
                if not glo.NoSimplify:
                    outf.write("#NESTING LEVEL L%i\n" % CurLev)
            outf.write("%i = %s(%s)\n" % 
                (RepVar, Expr[0], ", ".join([str(lit) for lit in sorted(Expr[1:], key=abs)])))
        if (PrStage): sys.stderr.write("Done writing output file, %.4f sec\n" % elapsed_time())


    ############################################################
    def PrintQdimacs(p, outf):
        #ChkUnquantVars(p)
        OutIsConj = (p.OutGlit > 0)
        if (not OnlyAnd):
            die("You must give the option '--no-allow-or' for QDIMACS output.")
        assert(OnlyAnd)

        TopClauseGates = set()
        if OutIsConj:
            TopClauseGates = set([abs(x) for x in p.RepTable[p.OutGlit][1:] if (-x > 0 and p.IsGateLit(x))])
            for (GateVar, expr) in p.RepTable.items():
                if (GateVar == p.OutGlit): continue
                TopClauseGates -= set([abs(x) for x in expr[1:]])
                    
        CalcNumCl = 0
        for (GateVar, expr) in p.RepTable.items():
            if (GateVar == p.OutGlit): continue
            if OutIsConj and (GateVar in TopClauseGates):
                CalcNumCl += 1
            else:
                CalcNumCl += 1 + len(expr[1:])
        if OutIsConj: 
            TopLits = set([x for x in p.RepTable[p.OutGlit][1:] if abs(x) not in TopClauseGates])
        else:
            TopLits = set([p.OutGlit])
        CalcNumCl += len(TopLits)

        if glo.KeepVarNames:
            for (NewNum, OrigName) in p.get_list_new_to_old():
                outf.write("c VarName %3i : %s\n" % (NewNum, OrigName))

        if WriteOldMap:
            outf.write('\n' + textwrap.fill(
                "c MapNewToOld (NewVar, OldVar) : (" + 
                ", ".join("(%i,%i)" % pair for pair in sorted(p.MapNewToOld.items()))
                + ")", 80, subsequent_indent='c   ') + "\n\n")
        outf.write("p cnf %i %i\n" % (p.LastGateVar, CalcNumCl))
        
        FlatBlks = []
        for (qgate, blks) in SortedQuantPfx(p):
            for quant in blks:
                if len(quant.qvars)==0: continue  # no variables quantified
                FlatBlks.append(quant)

        for quant in FlatBlks:
            outf.write(quant.qtype.lower() + " " + 
                (" ".join([str(lit).lower() for lit in quant.qvars])))
            if (quant is FlatBlks[-1]):
                RepKeys = set(p.RepTable.keys()) - TopClauseGates
                if (OutIsConj): RepKeys.remove(abs(p.OutGlit))
                if quant.qtype.lower() == 'e':
                    outf.write(" " + " ".join(str(x) for x in sorted(RepKeys)))
                else:
                    outf.write(" 0\n")
                    outf.write("e " + " ".join(str(x) for x in sorted(RepKeys)))
            outf.write(" 0\n")

        ActNumCl = 0
        
        def sxlat(x):
            return str(x)
            #if x>0: return str(p.MapNewToOld[x])
            #else:   return str(-p.MapNewToOld[abs(x)])   

        for lit in TopLits:
            outf.write(sxlat(lit) + " 0\n")
            ActNumCl += 1

        for (GateVar, conj) in reversed(sorted(p.RepTable.items())):
            # (v <==> (x1 & x2 & x3)) expands to
            # (~x1 | ~x2 | ~x3 | v)  &  (~v | x1)  &  (~v | x2)  &  (~v | x3)
            assert(conj[0] == 'and')
            args = conj[1:]
            if OutIsConj and (GateVar == p.OutGlit): continue
            if OutIsConj and (GateVar in TopClauseGates):
                outf.write(" ".join([sxlat(-x) for x in args]) + " 0\n")
                ActNumCl += 1
            else:
                outf.write(sxlat(GateVar) + " " + " ".join([sxlat(-x) for x in args]) + " 0\n")
                ActNumCl += 1
                for lit in args:
                    outf.write(sxlat(-GateVar) + " " + sxlat(lit) + " 0\n")
                    ActNumCl += 1

        assert(ActNumCl == CalcNumCl)

    ############################################################

    def doit():
        prob = read_input_file(filename)
        prob = TransmogrifyProb(prob)
        if (OutFilename != ""): 
            outf = open(OutFilename, 'w')
        else:
            outf = sys.stdout
        if glo.OutputFmt == 'qdimacs':
            PrintQdimacs(prob, outf)
        elif glo.OutputFmt == 'qcir':
            PrintQcir(prob, outf)
        elif glo.OutputFmt == 'cqbf':
            PrintGhostQ(prob, outf)
        else:
            die("Internal error: Unrecognized output format.")

    if 0: doit()
    else:
        try: doit()
        except KeyboardInterrupt:
            print "BREAK!"
            sys.exit(3)

main()
