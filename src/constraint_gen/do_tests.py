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

import os, sys, subprocess
import argparse
import json
import pdb
stop = pdb.set_trace

script_dir = os.path.relpath(os.path.dirname(os.path.abspath(__file__)), start=os.getcwd())
test_dir = script_dir + "/../test"

def add_newline_if_absent(s):
    if len(s) == 0:
        return s
    if s.endswith("\n"):
        return s
    else:
        return s + "\n"


def main():
    parser = argparse.ArgumentParser(description='Run tests')
    parser.add_argument('input', help='Input list of tests or a single test literal')
    parser.add_argument('-o', '--out-dir', type=str, help='Output directory')
    parser.add_argument('-f', '--write-fails', type=str, help='Write failures to this file')
    parser.add_argument('-r', '--write-results', type=str, help='Write results to this file')
    parser.add_argument('-t', '--test-id', nargs="+", type=str, help='Only do the test(s) with the given ID(s)')
    parser.add_argument('-c', '--unsat-core', action="store_true", help='Get unsat core')
    parser.add_argument('--warn-phis', type=int, default=1, help='Warn about phis missing metadata')
    parser.add_argument('--no-pom-yml', action="store_true", help='Do not use the program-specific ".pom.yml" file')
    parser.add_argument('--just-print-cmd', action="store_true", help='Just print the command for the test')
    parser.add_argument('--just-print-names', action="store_true", help='Just print the IDs of the tests')
    args = parser.parse_args()

    
    if args.out_dir:
        out_dir = args.out_dir
    else:
        out_dir = script_dir + "/out"
    os.makedirs(out_dir, exist_ok=True)

    if args.input.startswith("["):
        testcases = [json.loads(args.input)]
    else:
        with open(args.input, 'r') as infile:
            testcases = json.load(infile)
            testcases = [x for x in testcases if x[0] != "SKIP"]

    warn_phis_opt = ("" if args.warn_phis else "-no-warn-phis")
        

    fail_file = None
    if args.write_fails:
        fail_file = open(args.write_fails, "w")

    results_file = None
    if args.write_results:
        results_file = open(args.write_results, "w")

    if not args.just_print_names:
        sys.stdout.write("Compiling..."); sys.stdout.flush()
        exec_result = subprocess.run(f"cd {script_dir}; make", shell=True, capture_output=True, text=True)
        if exec_result.returncode == 0:
            sys.stdout.write("\n")
        else:
            sys.stderr.write("Error compiling!\n")
            sys.stderr.write(exec_result.stdout + "\n" + exec_result.stderr + "\n")
            sys.exit(1)

    next_suffix = {}
    
    errors = []
    for [expected_result, source_file, opts] in testcases:
        if "why" in opts:
            del opts["why"]
        testcase_json = json.dumps([expected_result, source_file, opts])
        src_short_name = source_file[:6]
        sfx = next_suffix.setdefault(src_short_name, 1)
        next_suffix[src_short_name] += 1
        test_id = src_short_name + f"{sfx:02}"
        if args.test_id and test_id not in args.test_id:
            continue
        if "D" in opts:
            defines = " ".join(["-D" + d for d in opts["D"]])
        else:
            defines = ""
        source_file = os.path.join(test_dir, source_file)
        base_path = os.path.splitext(source_file)[0]
        famous_pom = f"{test_dir}/test.famous.llm.pom.yml"
        if args.no_pom_yml and expected_result != "GOOD" and ("FUNC" in opts):
            del opts["FUNC"]
        if os.path.isfile(base_path + ".pom.yml") and not args.no_pom_yml:
            pom_file = base_path + ".pom.yml" + " " + famous_pom
        else:
            pom_file = famous_pom
        ll_file = base_path + ".raw.ll"
        func = opts.get("FUNC", "__all_funcs")
        llvm_opts = (opts.get("LLVM_OPTS", "") + " -glo-uniq " + warn_phis_opt + " " + os.getenv("LLVM_OPTS", "")).strip()
        command = (
            f'{script_dir}/compile_to_ll.sh {source_file} -o {ll_file} -c {defines} && ' +
            f'LLVM_OPTS="{llvm_opts}" FUNC="{func}" POM_FILE="{pom_file}" {script_dir}/run.sh {ll_file} -o {out_dir}'
        )
        command = command.replace("./../", "../")
        if args.just_print_cmd:
            print(command.replace(" && ", "\n"))
            continue
        sys.stdout.write(f"{test_id:8} : Running {testcase_json}... ")
        sys.stdout.flush()
        if args.just_print_names:
            sys.stdout.write("\n")
            continue
        exec_result = subprocess.run(command, shell=True, capture_output=True, text=True)
        output = exec_result.stdout.strip().split("\n")
        if len(output) < 2:
            is_sat = "ERROR"
        else:
            is_sat = output[-2]
            if is_sat == "SATISFIABLE":
                is_sat = True
            elif is_sat == "UNSATISFIABLE":
                is_sat = False
            else:
                is_sat = "ERROR"
        if is_sat == "ERROR":
            errors.append(
                f"Error running {testcase_json}!\n" +
                exec_result.stdout + "\n" +
                exec_result.stderr + "\n\n"
            )
            sys.stdout.write("ERROR\n")
            continue
        if expected_result not in ["GOOD", "BAD", "MLEAK"]:
            errors.append(f"Invalid expected result for {testcase_json}!\n")
        expected_result = (expected_result == "GOOD")
        bool_to_good_or_bad = {True: "GOOD", False: "BAD", "ERROR": "ERROR"}
        (expect_str, actual_str) = (bool_to_good_or_bad.get(x, "???") for x in [expected_result, is_sat])
        pass_or_fail = ("pass" if (is_sat == expected_result) else f"FAIL (expecting {expect_str}, got {actual_str})")
        sys.stdout.write(pass_or_fail + "\n")
        if fail_file and pass_or_fail != "pass":
            fail_file.write(f"{test_id:8} : {testcase_json}: {pass_or_fail}\n")
        if results_file:
            results_file.write(f"{test_id:8} : {testcase_json}: {pass_or_fail}\n")
        if len(exec_result.stderr.strip()) > 0:
            sys.stdout.write(add_newline_if_absent(exec_result.stderr))
        if is_sat==False and args.unsat_core:
            exec_result = subprocess.run(command.replace("/run.sh", "/get_unsat_core.sh") + " --collapse-flow", shell=True, capture_output=True, text=True)
            try:
                with open(f"{out_dir}/core.unsat.named", "r") as unsat_core:
                    sys.stdout.write("\n")
                    for line in unsat_core:
                        sys.stdout.write(line)
            except:
                sys.stdout.write("No core.unsat.named!\n")
            if exec_result.returncode != 0:
                sys.stdout.write(exec_result.stderr + "\n")
                sys.stdout.write(exec_result.stdout + "\n")

            
    sys.stderr.write("".join(errors))



if __name__ == '__main__':
    main()
