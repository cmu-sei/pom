#!/usr/bin/env python3
# <legal>
# Pointer Ownership Model (POM) Source Code Release
# 
# Copyright 2025 Carnegie Mellon University.
# 
# NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
# INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
# UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
# IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
# FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
# OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
# MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
# TRADEMARK, OR COPYRIGHT INFRINGEMENT.
# 
# Licensed under a MIT (SEI)-style license, please see license.txt or
# contact permission@sei.cmu.edu for full terms.
# 
# [DISTRIBUTION STATEMENT A] This material has been approved for public
# release and unlimited distribution.  Please see Copyright notice for
# non-US Government use and distribution.
# 
# DM25-1262
# </legal>

import argparse
import sys, os
import json
import re
import pdb
import traceback
import subprocess
from collections import namedtuple
from collections import defaultdict

stop = pdb.set_trace

script_dir = os.path.relpath(os.path.dirname(os.path.abspath(__file__)), start=os.getcwd())

global cmdline_args

Clause = namedtuple('Clause', ['lits', 'priority', 'comment'])

pri_cat_levels = [
    ["flow", "basic"],
    ["ptr_copy"],
    ["ali_ptr_copy"],
    ["uninit", "mut", "move_resp", "copy_zombie", "zombie_arg"],
    ["deref_zombie", "mut_via_immut", "resp_from_irresp", "double_free", "invalid_free"],
    ["borrow"],
    ["null_deref"],
    ["mem_leak"],
    ["caller_demands"],
]
pri_cat_to_pri_num = {}
for (idx, categories) in enumerate(pri_cat_levels):
    for cat in categories:
        pri_cat_to_pri_num[cat] = idx + 1

comment_to_pri_cat = {}

def load_priority_comments():
    ConstraintGenPass = script_dir + "/" + "ConstraintGenPass.cpp"
    exec_result = subprocess.run(f'grep "pri_cat=[A-Za-z0-9_]*" {ConstraintGenPass}',
        shell=True, capture_output=True, text=True)
    lines = exec_result.stdout.split("\n")
    for line in lines:
        m = re.match("^.*pri_cat=([A-Za-z0-9_]*)", line.strip())
        if not m:
            continue
        pri_cat = m.group(1)

        m = re.match("^[^#]* # ([A-Za-z0-9' ]*)", line.strip())
        if not m:
            continue
        comment = m.group(1)

        comment_to_pri_cat[comment] = pri_cat

def get_pri_cat_from_comment(given_comment):
    for (it_comment, it_pri_cat) in comment_to_pri_cat.items():
        if given_comment.startswith(it_comment):
            return it_pri_cat
    return None

def subst_under_asgn(clause, asgn):
    ret = []
    for lit in clause:
        val = asgn.get(lit)
        if val is True:
            return True
        if val is False:
            continue
        else:
            ret.append(lit)
    if ret == []:
        return False
    return ret

def var_of_lit(lit):
    if lit.startswith("!"):
        var = lit[1:]
    else:
        var = lit
    return var

def negate_lit(lit):
    if lit.startswith("!"):
        return lit[1:]
    else:
        return "!" + lit

def is_neg_lit(lit):
    return lit.startswith("!")

forced_lit_color_code = "0;255;0"

def color_forced_lit(text):
    if forced_lit_color_code == 'none':
        return text
    return "\033[38;2;" + forced_lit_color_code + "m" + text + "\033[0m"

def main(argv=None):
    parser = argparse.ArgumentParser(description='Examines unsat core')
    parser.add_argument("input_file", type=str, help="Input file (unsat core)")
    parser.add_argument("--keep-filename", action="store_true", help="Don't remove the filename from var names")
    parser.add_argument("--color-forced-lit", type=str, help="Color for forced literal (format: 'R;G;B' or 'none') (default: '0;255;0')")
    parser.add_argument("-a", "--write-asgn", type=str, help="Mode to generate an assignment and write to specified file ('-' for stdout)")
    parser.add_argument("-s", "--show-forcing", action="store_true", help="Show forcing clauses even in assignment mode")
    cmdline_args = args = parser.parse_args(argv)

    if not args.write_asgn:
        args.show_forcing = True

    if args.color_forced_lit:
        global forced_lit_color_code
        forced_lit_color_code = args.color_forced_lit

    if args.keep_filename:
        def shorten(lit):
            return lit
    else:
        def shorten(lit):
            m = re.match("([^,]*, ).*_(L[0-9]+([a-zA-Z0-9_-]+)_I[0-9]+-[a-z]+[)])$", lit)
            if m:
                return m.group(1) + m.group(2)
            else:
                return lit
    load_priority_comments()
    clause_list = read_unsat_core_file(args.input_file)
    #for clause in clause_list:
    #    print(clause)
    max_pri = len(pri_cat_levels)
    clauses_by_pri = defaultdict(list)
    all_vars = set()
    for clause in clause_list:
        clauses_by_pri[clause.priority].append(clause)
        for lit in clause.lits:
            all_vars.add(var_of_lit(lit))
    cur_asgn = {}
    forced_by = {}
    is_done = False
    is_dirty = True
    if not args.write_asgn:
        print("\n" + ("#" * 80))
    next_cl_num = 1
    seen_falsified_clauses = set()
    while True:
        if (is_done and not args.write_asgn) or not is_dirty:
            break
        is_dirty = False
        for cur_pri in range(1, max_pri+1):
            if is_dirty:
                break
            for cur_cl in clauses_by_pri[cur_pri]:
                reduced_cl = subst_under_asgn(cur_cl.lits, cur_asgn)
                if reduced_cl is True:
                    continue
                if reduced_cl is False:
                    if tuple(cur_cl.lits) not in seen_falsified_clauses:
                        print(f"{next_cl_num:3}: Falsified: " + " | ".join(f"{forced_by[x]}:" + shorten(x) for x in cur_cl.lits) + " # " + cur_cl.comment) # + f" # Pri {cur_cl.priority}"
                        next_cl_num += 1
                        seen_falsified_clauses.add(tuple(cur_cl.lits))
                    if args.write_asgn:
                        continue
                    is_done = True
                    is_dirty = True
                    break
                if len(reduced_cl) == 1:
                    forced_lit = reduced_cl[0]
                    cur_asgn[forced_lit] = True
                    cur_asgn[negate_lit(forced_lit)] = False
                    forced_by[forced_lit] = next_cl_num
                    forced_by[negate_lit(forced_lit)] = next_cl_num
                    is_dirty = True
                    if args.show_forcing:
                        clause_text = " | ".join(("forced:"+color_forced_lit(shorten(lit)) if lit == forced_lit else f"{forced_by[lit]}:" + shorten(lit)) for lit in cur_cl.lits)
                        print(f"{next_cl_num:3}: {clause_text} # " + cur_cl.comment) # + f" # Pri {cur_cl.priority}"
                    next_cl_num += 1
                    #print(f"Literal {forced_lit} forced by clause: " + clause_text + " # " + cur_cl.comment)
                    break

    if (not is_done) and (not args.write_asgn):
        print("\nRemaining clauses:")
        for cur_cl in clause_list:
            print(" | ".join(cur_cl.lits) + " # " + cur_cl.comment)
            #reduced_cl = subst_under_asgn(cur_cl.lits, cur_asgn)
    
    if args.write_asgn:
        unassigned_vars = set(all_vars)
        def is_dummy_var(var):
            return re.match("^[a-z]+[(]NULL_CONST,", var)
        def write_asgn_to_file(outfile):
            for (lit, sign) in cur_asgn.items():
                if is_neg_lit(lit):
                    continue
                if not is_dummy_var(lit):
                    outfile.write(f"{lit} = {sign}\n")
                unassigned_vars.discard(lit)
            for lit in sorted(unassigned_vars):
                if not is_dummy_var(lit):
                    outfile.write(f"{lit} = unassigned\n")
        if args.write_asgn in ["-", "stdout"]:
            write_asgn_to_file(sys.stdout)
        else:
            with open(args.write_asgn, "w") as outfile:
                write_asgn_to_file(outfile)
                
                    



def read_unsat_core_file(filename):
    clause_list = []
    with open(filename, 'r') as in_file:
        line_num = 0
        for line in in_file:
            line_num += 1
            line = line.strip()
            if line.strip() == "" or line.startswith("#") or line.startswith("<"):
                continue
            comment = ""
            if "#" in line:
                (line, comment) = [x.strip() for x in line.split("#", 1)]
            if ("|" not in line) and (" -> " in line):
                parts = line.split(" -> ")
                if len(parts) == 2:
                    (antecedent, consequent) = parts
                    line = "!" + antecedent.strip() + " | " + consequent.strip()
                else:
                    sys.stderr.write(f"Warning: line {line_num} is malformed: '{line}'\n")
                    continue
            clause_lits = [x.strip() for x in line.split("|")]
            if comment:
                comment_lower = comment.lower()
                if comment_lower in ["flow", "preservation", "combined flow"]:
                    pri_cat = "flow"
                elif "leak" in comment:
                    pri_cat = "mem_leak"
                elif comment.__contains__("PtrCopy"):
                    if "Callsite" in comment and "::" in line:
                        pri_cat = "caller_demands"
                    elif "(alias)" in comment:
                        pri_cat = "ali_ptr_copy"
                    else:
                        pri_cat = "ptr_copy"
                elif comment == "All responsible pointers are mutable":
                    pri_cat = "basic"
                else:
                    pri_cat = get_pri_cat_from_comment(comment)
                    if pri_cat is None:
                        pri_cat = "basic"
            else:
                pri_cat = "basic"
            priority = pri_cat_to_pri_num.get(pri_cat, None)
            if priority is None:
                sys.stderr.write(f"Warning: Unknown priority category '{pri_cat}'.\n")
                priority = 2
            #print((pri_cat, priority))
            clause_list.append(Clause(clause_lits, priority, comment))
    return clause_list


if __name__ == '__main__':
    main()
