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
import shlex
import time

stop = pdb.set_trace

class DummyException(Exception):
    pass

def parse_args(argv):
    parser = argparse.ArgumentParser(description='Convert to DIMACS for SAT solver or convert solution back to named vars',
        exit_on_error = not in_coproc_mode)
    parser.add_argument("input_file", type=str, help="Input file")
    parser.add_argument("-v", type=str, required=True, dest="map_file", help="File for mapping variables names <-> numbers")
    parser.add_argument('-o', type=str, metavar="OUTPUT_FILE", dest="output_file", required=True, help="Output file")
    parser.add_argument('--func', dest="func", type=str, help="Name of function to process")
    parser.add_argument('--summarize',  action="store_true", help="Treat the args and ret as free vars, not existential")
    parser.add_argument('--to-dimacs', dest="to_dimacs", action="store_true", help="Convert to DIMACS")
    parser.add_argument('--from-dimacs', dest="from_dimacs", action="store_true", help="Convert from DIMACS")
    parser.add_argument('--from-sol', dest="from_sol", action="store_true", help="Convert solution from numbered vars to named vars")
    parser.add_argument('--clause-info-file', type=str, help="Get clause metadata from constraints.txt file")
    parser.add_argument('--collapse-flow', action="store_true", help="Collapse preserved properties")
    parser.add_argument('--catch-exc', action="store_true", help="Catch exceptions and print invocation.")
    parser.add_argument('--coproc', action="store_true", help="Co-process")
    cmdline_args = parser.parse_args(argv)
    return cmdline_args

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

def read_whole_file(filename):
    with open(filename, 'r') as the_file:
        return the_file.read()

in_coproc_mode = False

def run_as_coproc():
    global in_coproc_mode
    in_coproc_mode = True
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        if line in ["quit", "exit"]:
            return
        #start_time = time.time()
        retcode = main(shlex.split(line) + ["--catch-exc"])
        #sys.stderr.write("conv_dimacs time: %1.3f s\n" % (time.time() - start_time))
        if retcode == 0:
            sys.stdout.write("FINISHED\n")
        else:
            sys.stdout.write("ERROR\n")
        sys.stdout.flush()

def main(argv=None):
    if argv==None and len(sys.argv) >= 2 and sys.argv[1] == "--coproc":
        run_as_coproc()
        return 0
    try:
        cmdline_args = parse_args(argv)
    except:
        return 1
    ExceptionToCatch = (Exception if cmdline_args.catch_exc else DummyException)
    try:
        if cmdline_args.to_dimacs or cmdline_args.output_file.endswith(".dimacs"):
            conv_to_dimacs(cmdline_args)
        elif cmdline_args.from_sol:
            conv_sol(cmdline_args)
        elif cmdline_args.from_dimacs:
            clause_info = get_clause_info_from_file(cmdline_args)
            conv_from_dimacs(cmdline_args, clause_info)
        else:
            sys.stderr.write("Must specify '--to-dimacs', '--from-dimacs', or '--from-sol'\n")
            return 1
    except ExceptionToCatch as e:
        sys.stderr.write(str(e) + "\n")
        traceback.print_exc()
        sys.stderr.write("\nCommand invocation:\n" + " ".join(sys.argv) + "\n\n")
        return 1
    return 0
        

def get_clause_info_from_file(cmdline_args):
    clause_info = {}
    if cmdline_args.clause_info_file:
        class DummyArgs:
            pass
        dummy_args = DummyArgs()
        dummy_args.func = cmdline_args.func
        dummy_args.input_file = cmdline_args.clause_info_file
        conv_to_dimacs(dummy_args, clause_info)
    else:
        sys.stderr.write("Warning: command-line argument '--clause-info-file <path/to/constraints.txt>' is missing!\n")
    return clause_info

def conv_to_dimacs(cmdline_args, clause_info=None):
    var_name_to_num = {}
    p_next_var_num = [1]

    def add_lit_to_map(lit):
        var = var_of_lit(lit)
        if var not in var_name_to_num:
            var_name_to_num[var] = p_next_var_num[0]
            p_next_var_num[0] += 1

    def lit_name_to_num(lit):
        var = var_of_lit(lit)
        var_num = var_name_to_num[var]
        if var == lit:
            lit_num = var_num
        else:
            lit_num = -var_num
        return lit_num

    target_func_name = cmdline_args.func or "__all_funcs"

    clause_list = []
    inside_func = False
    line_num = 0
    cur_func_name = ""
    in_target_func = False
    found_function = False
    in_yaml_file = False
    with open(cmdline_args.input_file, 'r') as in_file:
        (opcode, callee, loc) = (None, None, None)
        clause_type = None
        phi_incoming = None
        for line in in_file:
            line_num += 1
            line = line.strip()
            if line.strip() == "" or line.startswith("#"):
                if line.startswith("# Flow"):
                    clause_type = "flow"
                if line.startswith("# Preservation"):
                    clause_type = "preservation"
                if line.startswith("# End of preservation"):
                    clause_type = None
                if line.startswith("# Killed variables"):
                    clause_type = "killed_var"
                if line.startswith("# Phi incoming: "):
                    clause_type = None
                    phi_incoming = line.replace("# Phi incoming: ", "")
                if line.startswith("# All responsible pointers are mutable"):
                    clause_type = "All responsible pointers are mutable"
                if line.startswith("# Function exit point"):
                    clause_type = None
                continue
            comment = None
            if " #" in line:
                comment = line[line.index(" #")+len(" #"):].strip()
                line = line[:line.index(" #")].strip()

            m = re.match('<function name="([^"]*)">', line.strip())
            if m:
                if inside_func:
                    sys.stderr.write(f"Error on line {line_num}: already inside a function!\n")
                    continue
                else:
                    inside_func = True
                    cur_func_name = m.group(1)
                    in_target_func = (cur_func_name == target_func_name) or (target_func_name == "__all_funcs")
                    if in_target_func:
                        found_function = True
                    continue

            m = re.match('</function>', line.strip())
            if m:
                clause_type = None
                if not inside_func:
                    sys.stderr.write(f"Error on line {line_num}: not inside a function!\n")
                    continue
                else:
                    inside_func = False
                    continue
            
            if line == "<yaml_file>":
                clause_type = None
                in_target_func = True
                in_yaml_file = True
                continue
            
            if line == "</yaml_file>":
                in_target_func = False
                in_yaml_file = False
                continue
                
            if line == "<common>":
                in_target_func = True
                continue
            
            if line == "</common>":
                in_target_func = False
                continue
                
            if line.startswith("<BasicBlock") or line == "</BasicBlock>":
                clause_type = None
                continue
            
            m = re.match('<instruction opcode="([^"]*)" (callee="([^"]*)" )?loc="([^"]*)">', line.strip())
            if m:
                (opcode, callee, loc) = (m.group(x) for x in [1, 3, 4])
                clause_type = None
                phi_incoming = None
                continue

            m = re.match('</instruction>', line.strip())
            if m:
                (opcode, callee, loc) = (None, None, None)
                clause_type = None
                continue

            if in_target_func:
                if ("|" not in line) and (" -> " in line):
                    parts = line.split(" -> ")
                    if len(parts) == 2:
                        (antecedent, consequent) = parts
                        line = "!" + antecedent.strip() + " | " + consequent.strip()
                    else:
                        sys.stderr.write(f"Warning: line {line_num} is malformed: '{line}'\n")
                lits = [x.strip() for x in line.split("|")]
                if not (clause_info is None):
                    cur_clause_type = clause_type
                    if comment == "killed_var":
                        cur_clause_type = "killed_var"
                        comment = ""
                    if comment == "Preservation":
                        cur_clause_type = "preservation"
                        comment = ""
                    cur_info = {
                        "clause": lits,
                        "inst_opcode": opcode,
                        "inst_loc": loc,
                        "clause_type": cur_clause_type,
                    }
                    if callee:
                        cur_info["inst_callee"] = callee
                    if in_yaml_file and comment==None:
                        comment = "From POM YAML file"
                    if opcode=="call" and ("PtrCopy" in (comment or "")):
                        comment += " # Callsite"
                    if comment:
                        cur_info["comment"] = comment
                    if phi_incoming:
                        cur_info["phi_incoming"] = phi_incoming
                    clause_info.setdefault(tuple(sorted(lits)), []).append(cur_info)
                for lit in lits:
                    add_lit_to_map(lit)
                    var = var_of_lit(lit)
                    if any(var.startswith(x) for x in ["good(", "null(", "zombie("]):
                        if ", " not in var:
                            sys.stderr.write(f"Missing time point: {var}\n")
                    else:
                        if "," in var:
                            if not var.startswith("borrows_from"):
                                sys.stderr.write(f"Spurious time point: {var}\n")
                lit_nums = [lit_name_to_num(lit) for lit in lits]
                clause_list.append(lit_nums)
    
    if not (clause_info is None):
        return

    num_vars = p_next_var_num[0] - 1
    num_clauses = len(clause_list)
    with open(cmdline_args.output_file, 'w') as out_file:
        free_vars = []
        existential_vars = []
        if cmdline_args.summarize:
            for (vname, vnum) in var_name_to_num.items():
                is_free = False
                if re.match("[A-Za-z_]*[(][A-Za-z0-9_:*]*::args::[^), ]*[)]", vname):
                    is_free = True
                if re.match("[A-Za-z_]*[(][A-Za-z0-9_]*::return[)]", vname):
                    is_free = True
                if re.match("[A-Za-z_]*[(][A-Za-z0-9_:*]*::args::[^), ]*, start[)]", vname):
                    is_free = True
                if re.match("[A-Za-z_]*[(][A-Za-z0-9_]*::return, end[)]", vname):
                    is_free = True
                if is_free:
                    free_vars.append(str(vnum))
                else:
                    existential_vars.append(str(vnum))
                vname_mangled = vname
                vname_mangled = vname_mangled.replace(":", ".")
                #vname_mangled = re.sub("arg:([0-9])+:", r"arg.\1.", vname_mangled)
                vname_mangled = vname_mangled.replace("arg:", "arg.")
                vname_mangled = vname_mangled.replace(", ", "_@_")
                vname_mangled = vname_mangled.replace("(", "<")
                vname_mangled = vname_mangled.replace(")", ">")
                out_file.write(f"c VarName {vnum} : {vname_mangled}\n")
        out_file.write(f"p cnf {num_vars} {num_clauses}\n")
        if cmdline_args.summarize:
            out_file.write("f " + " ".join(free_vars) + " 0\n")
            out_file.write("e " + " ".join(existential_vars) + " 0\n")

        if len(clause_list) == 0:
            sys.stderr.write(f"Warning: No clauses generated!\n")
        if not found_function:
            sys.stderr.write(f"Error: function '{target_func_name}' not found!\n")

        for clause in clause_list:
            out_file.write(" ".join(str(lit_num) for lit_num in clause) + " 0\n")
            
    with open(cmdline_args.map_file, 'w') as map_file:
        map_file.write(json.dumps(var_name_to_num, indent=2) + "\n")

def conv_sol(cmdline_args):
    tokens = read_whole_file(cmdline_args.input_file).split()
    assert(tokens[0] == "SAT")
    assert(tokens[-1] == "0")
    with open(cmdline_args.map_file, 'r') as map_file:
        name_to_num = json.load(map_file)
    num_to_name = dict((num, name) for (name, num) in name_to_num.items())
    out_map = {}
    for lit_num in tokens[1:-1]:
        lit_num = int(lit_num)
        var_num = abs(lit_num)
        var_name = num_to_name[var_num]
        out_map[var_name] = (lit_num > 0)

    with open(cmdline_args.output_file, 'w') as out_file:
        out_file.write(json.dumps(out_map, indent=2) + "\n")

def resugar_pos_conditional(clause):
    if len(clause) != 2:
        return None
    if not clause[0].startswith("!"):
        clause = list(reversed(clause))
    if not clause[0].startswith("!"):
        return None
    if clause[1].startswith("!"):
        return None
    return clause[0][1:] + " -> " + clause[1]
    

def collapse_flow(all_clauses, clause_info):
    
    def get_clause_info(clause):
        info_list = clause_info.get(tuple(sorted(clause)))
        if info_list:
            info = info_list[0]
            return info
        else:
            return {}

    # Precomputation
    var_to_containing_clauses = {}
    for clause in all_clauses:
        for lit in clause:
            var_name = var_of_lit(lit)
            canonical_clause = tuple(sorted(clause))
            var_to_containing_clauses.setdefault(var_name, set()).add(canonical_clause)
    
    def next_step_clause(clause, dir):
        assert(dir in [0,1])
        info = get_clause_info(clause)
        if info.get('clause_type') not in ["flow", "preservation"]:
            return None
        clause = info["clause"]
        canonical_clause = tuple(sorted(clause))
        lit = clause[dir]
        containing_clauses = list(var_to_containing_clauses[var_of_lit(lit)])
        assert(canonical_clause in containing_clauses)
        containing_clauses.remove(canonical_clause)
        if len(containing_clauses) != 1:
            return None
        other_clause = containing_clauses[0]
        other_info = get_clause_info(other_clause)
        if other_info.get("clause_type") not in ["flow", "preservation"]:
            return None
        other_lits = list(x for x in other_clause if x not in [var_of_lit(lit), "!" + var_of_lit(lit)])
        assert(len(other_lits) == 1)
        assert(other_lits[0] == other_clause[dir])
        return other_clause

    del_clauses = []

    def end_of_flow_chain(clause, dir, hit_clauses):
        other_clause = next_step_clause(clause, dir)
        if not other_clause:
            return None
        while True:
            hit_clauses.add(other_clause)
            next_cl = next_step_clause(other_clause, dir)
            if not next_cl:
                break
            assert(next_cl not in hit_clauses)
            assert(next_cl not in del_clauses)
            other_clause = next_cl
        return other_clause
    
    for (ix, clause) in enumerate(all_clauses):
        hit_clauses = set()
        if tuple(sorted(clause)) in del_clauses:
            continue
        info = get_clause_info(clause)
        if not info:
            continue
        clause = info["clause"]
        rear_clause = end_of_flow_chain(clause, 1, hit_clauses)
        if not rear_clause:
            continue
        front_clause = end_of_flow_chain(clause, 0, hit_clauses)
        if not front_clause:
            front_clause = clause
        hit_clauses.add(tuple(sorted(clause)))
        del_clauses.extend(hit_clauses)
        meta_clause = (negate_lit(front_clause[0]), rear_clause[1])
        cur_info = {
            "clause": meta_clause,
            "clause_type": "combined_flow",
        }
        clause_info[tuple(sorted(meta_clause))] = [cur_info]
        #print("meta_clause: ", meta_clause)
        #print("hit_clauses: ", hit_clauses)
        all_clauses.append(meta_clause)

    all_clauses = [cl for cl in all_clauses if tuple(sorted(cl)) not in del_clauses]
    return all_clauses

        
def conv_from_dimacs(cmdline_args, clause_info):
    # Read the variable mapping
    with open(cmdline_args.map_file, 'r') as map_file:
        name_to_num = json.load(map_file)
    
    # Create reverse mapping from number to name
    num_to_name = dict((num, name) for (name, num) in name_to_num.items())
    
    # Read and convert DIMACS file
    all_clauses = []
    with open(cmdline_args.input_file, 'r') as in_file:
        for line in in_file:
            line = line.strip()
            # Skip comment lines and empty lines
            if line.startswith('c') or not line:
                continue
            # Skip the problem line
            if line.startswith('p cnf'):
                continue
            
            # Parse clause
            tokens = line.split()
            # Remove the trailing 0
            if tokens and tokens[-1] == '0':
                tokens = tokens[:-1]
            
            # Convert each literal number back to named variable
            named_literals = []
            for token in tokens:
                lit_num = int(token)
                var_num = abs(lit_num)
                var_name = num_to_name[var_num]
                
                if lit_num < 0:
                    named_literal = "!" + var_name
                else:
                    named_literal = var_name
                
                named_literals.append(named_literal)
            all_clauses.append(named_literals)

    # Collapse flow, if requested
    if cmdline_args.collapse_flow or (os.getenv("COLLAPSE_FLOW", "").lower() in ["1", "true"]):
        all_clauses = collapse_flow(all_clauses, clause_info)

    # Process and print each clause
    with open(cmdline_args.output_file, 'w') as out_file:
        for clause in all_clauses:
            named_literals = clause
            info_list = clause_info.get(tuple(sorted(named_literals)))
            if info_list:
                info = info_list[0]
                if info.get("clause"):
                    named_literals = info["clause"]
            else:
                info = {}
                sys.stderr.write("Warning: missing metadata for clause %r\n" % (named_literals,))
            #out_file.write("\n# " + repr(info) + "\n")
            comment = info.get("comment", "")
            if comment:
                if ("(via phi)" in comment) and info.get("phi_incoming"):
                    comment = comment.replace("(via phi)", "(via phi, incoming from " + info["phi_incoming"] + ")")
                comment = " # " + comment
            else:
                if len(named_literals) == 1:
                    if named_literals[0].startswith("responsible"):
                        callee = info.get("inst_callee")
                        if callee in ["free", "malloc", "calloc"]:
                            comment = " # " + callee + " at " + info.get("inst_loc")
            if (info.get("inst_loc","") or "") not in (" | ".join(named_literals) + " # " + comment):
                comment += " # " + info["inst_loc"]
            
            # Write the clause
            if named_literals:  # Only write non-empty clauses
                sugared_clause = None
                if info.get("clause_type") == "All responsible pointers are mutable":
                    comment += " # All responsible pointers are mutable"
                if info.get("clause_type") == "killed_var":
                    comment += " # Memory leak: good responsible pointer becomes dead"
                if info.get('clause_type') in ["flow", "preservation"]:
                    sugared_clause = resugar_pos_conditional(named_literals)
                    if sugared_clause is None:
                        sys.stderr.write(f"Warning: bad clause_type for {named_literals}?\n")
                    else:
                        sugared_clause += " # " + info['clause_type'].capitalize()
                if info.get("clause_type") == "combined_flow":
                    assert(len(named_literals) == 2)
                    out_file.write(f"{named_literals[0]} -> {named_literals[1]}" + comment + " # Collapsed flow\n")
                elif sugared_clause:
                    out_file.write(sugared_clause + "\n")
                else:
                    out_file.write(" | ".join(named_literals) + comment + "\n")

if __name__ == '__main__':
    main()
