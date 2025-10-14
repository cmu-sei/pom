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


import pytest
import subprocess
import json
import argparse
import os
import sys
import shlex
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
SRC_DIR = TEST_DIR.parent
sys.path.append(str(SRC_DIR))

import run_solver

def run_test_solver(basename, cmdline_args):
    out_dir = os.path.abspath(cmdline_args.out_dir)
    for ext in [".c", ".expect.json"]:
        if basename.endswith(ext):
            basename = basename[:-len(ext)]
    source_file = TEST_DIR / f"{basename}.c"
    pom_file = TEST_DIR / f"{basename}.pom.yml"
    ll_file = TEST_DIR / f"{basename}.raw.ll"
    expect_file = TEST_DIR / f"{basename}.expect.json"

    # Ensure that all the files we need are there
    assert source_file.is_file()

    if not ll_file.exists() or ll_file.stat().st_mtime < source_file.stat().st_mtime:
        result = subprocess.run(
            f"{SRC_DIR}/constraint_gen/compile_to_ll.sh {source_file} -o {ll_file}",
            shell=True,
            capture_output=True,
            text=True,
            cwd=TEST_DIR,
        )

    # Run the verifier
    num_fails = 0
    if expect_file.is_file() and pom_file.is_file() and ll_file.is_file():
        timing_arg = (f"--timings {out_dir}/timings.json" if cmdline_args.timings else "")
        py_proc_arg = (f"--new-py-proc" if cmdline_args.new_py_proc else "")
        args_for_run_solver = f"--json --pom-file {pom_file} --library-pom /host/src/test/test.famous.llm.pom.yml {ll_file} --no-unsat-core {timing_arg} {py_proc_arg} -o {out_dir}"
        cmd = f"python3 run_solver.py {args_for_run_solver}"
        raw_actual_text_output = None
        if cmdline_args.new_py_proc:
            result = subprocess.run(
                cmd,
                shell=True,
                capture_output=True,
                text=True,
                cwd=SRC_DIR,
            )
            raw_actual_text_output = result.stdout
        else:
            raw_actual_text_output = run_solver.call_and_capture_stdout(lambda: run_solver.main(shlex.split(args_for_run_solver)))
        raw_actual_output = {}
        actual_output = {}
        try:
            raw_actual_output = json.loads(raw_actual_text_output)
            actual_output = raw_actual_output["function-satisfiability"]
        except:
            print(f"Error reading JSON for {basename}\n")
            print(f"Command: {cmd}\n")

        # Read in the expected result
        try:
            with open(expect_file) as expect_file_handle:
                expected_output = json.load(expect_file_handle)["function-satisfiability"]
        except:
            print(f"Error reading expected output for {basename}")
            expected_output = {}

        all_funcs = sorted(set(list(actual_output.keys()) + list(expected_output.keys())))
        dirty = False
        for func in all_funcs:
            act = actual_output.get(func, "absent").replace("SATISFIABLE", "SAT")
            exp = expected_output.get(func, "absent").replace("SATISFIABLE", "SAT")
            if act != exp:
                print(f"❌ {basename}: FAIL: {func}: expected {exp}, got {act}")
                if cmdline_args.print_cmd:
                    print(f'   POM_FILE="{basename}.pom.yml test.famous.llm.pom.yml" ' +
                          f'FUNC="{func}" ../constraint_gen/get_unsat_core.sh {basename}.raw.ll -o ../constraint_gen/out')
                dirty = True
                num_fails += 1
        if not dirty:
            print(f" ✓ {basename}: pass")
        if cmdline_args.write_output_json:
            with open(out_dir + "/" + basename + ".results.json", "w") as out_file:
                out_file.write(json.dumps(raw_actual_output, indent=2) + "\n")

    else:
        print(f" - Skipping {basename}")

    return {"num_fails": num_fails}

def main():
    parser = argparse.ArgumentParser(description='Run tests')
    parser.add_argument('test_name', nargs="+", help='Test name(s) or ".c" file(s)')
    parser.add_argument('-o', '--out-dir', default="out", type=str, help='Output directory')
    parser.add_argument('--write-output-json', action="store_true", help='Write actual output JSON files')
    parser.add_argument('-c', '--print-cmd', action="store_true", help='Print command for running')
    parser.add_argument('--timings', action="store_true", help='Print timing data')
    parser.add_argument('--new-py-proc', action="store_true", help='Launch new Python process')
    args = parser.parse_args()

    timings_file = f"{args.out_dir}/timings.json"
    if args.timings and os.path.exists(timings_file):
        os.remove(timings_file)
        
    num_fails = 0
    for test_name in args.test_name:
        ret = run_test_solver(test_name, args)
        num_fails += ret["num_fails"]

    print(f"Number of failing tests: {num_fails}")

    if args.timings:
        with open(timings_file, "r") as infile:
            timing_data = json.load(infile)
        cum_timings = []
        total_time = 0
        for (action, times) in timing_data.items():
            cum_timings.append([sum(times), action, len(times)])
            total_time += sum(times)
        sys.stdout.write("\n")
        for (time, action, count) in sorted(cum_timings):
            print("%6.2f %s (count: %d)" % (time, action, count))
        if 1:
            print("%6.2f TOTAL TIME" % (total_time))



if __name__ == '__main__':
    main()
