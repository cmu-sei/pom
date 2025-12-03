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


import sys, os, re
import argparse
import time
from datetime import datetime
import subprocess
import hashlib

def read_whole_file(filename):
    with open(filename, 'r') as the_file:
        return the_file.read()

def add_newline_if_absent(s):
    if len(s.strip()) == 0:
        return ""
    if s.endswith("\n"):
        return s
    else:
        return s + "\n"

error_log_file = None

def add_line_nums(file_contents):
    out_lines = []
    lines = file_contents.splitlines()
    for idx, content in enumerate(lines, start=1):
        if content.endswith("\\") or len(content.strip()) == 0 or content.strip().startswith('#'):
            out_lines.append(content + "\n")
        else:
            out_lines.append(f"{content} // Line {idx}\n")
    return "".join(out_lines)

def collect_test_cases_from_file_list(list_of_juliet_files):
    """Maps test case names to their constituent files."""
    testcase_to_files = {}
    for filename in list_of_juliet_files:
        filename = filename.strip()
        if not (filename.endswith(".c") or filename.endswith(".cpp") or filename.endswith(".h")):
            continue
        parts = filename.split("/")
        if parts[0] != "testcases":
            continue

        m = re.match("^CWE([0-9]+)_.*$", parts[1])
        if not m:
            continue
        CWE_ID = m.group(1)

        s_num = ""
        if len(parts) == 4:
            m = re.match("(s[0-9]+)", parts[2])
            if m:
                s_num = m.group(1)

        m = re.match(".*__([A-Za-z0-9_]*)_([0-9][0-9]*)([a-z]|_[a-z][a-zA-Z0-9]*)?[.](c|cpp|h)", parts[-1])
        if not m:
            sys.stderr.write("Unrecognized file: " + repr(filename) + "\n")
            continue

        main_name = m.group(1)
        flow_variant = m.group(2)
        shortname = f"CWE{CWE_ID},{s_num},{main_name}_{flow_variant}"
        filename = "/".join(parts[1:])
        testcase_to_files.setdefault(shortname, set()).add(filename)

    return testcase_to_files

def run_testcase_half(testcase_name, testcase_files, config, script_dir, unsat_core=False):
    """Run a single half (GOOD or BAD) of a test case."""
    global cmdline_args
    testcase_files = [os.path.join(cmdline_args.testcases_dir, x) for x in testcase_files]
    cmd = [os.path.join(script_dir, "run_juliet_test_half.sh")]
    cmd.extend(sorted(testcase_files))
    cmd.append(config)
    if unsat_core:
        cmd.append("--unsat-core")

    env = os.environ.copy()
    env['TESTCASE_NAME'] = testcase_name
    env['NO_SSA_FILE'] = ("true" if not cmdline_args.testcase else "")
    env['SOL_JSON_ARG'] = ("--no-solution-json" if not cmdline_args.testcase else "")
    env['SINGLE_OR_MULTI'] = (" (M)" if len(testcase_files) > 1 else " (S)")

    extra_llvm_opts = " -no-pom-yaml-structs -no-pom-yaml-locals"
    if not cmdline_args.all_checks:
        if not testcase_name.startswith("CWE401"):
            extra_llvm_opts += " -no-mem-leak"
        if not testcase_name.startswith("CWE476"):
            extra_llvm_opts += " -no-null-check"
    env['LLVM_OPTS'] = env.get('LLVM_OPTS', '') + extra_llvm_opts

    coproc_fds = [os.getenv(x) for x in ["CONV_DIMACS_COPROC_STDIN", "CONV_DIMACS_COPROC_STDOUT"]]
    coproc_fds = [x for x in coproc_fds if x != None]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, env=env, check=True, pass_fds=coproc_fds)
        sys.stderr.write(add_newline_if_absent(result.stderr))
        if error_log_file:
            error_log_file.write(add_newline_if_absent(result.stderr))
        return result.stdout.rstrip()
    except subprocess.CalledProcessError as e:
        err_msg = (f"Error running {testcase_name} {config}:\n" +
            add_newline_if_absent(e.stdout) +
            add_newline_if_absent(e.stderr) + "\n")
        sys.stderr.write(err_msg)
        if error_log_file:
            error_log_file.write(err_msg)
        return f"!! ERROR: {config} version of {testcase_name}"

def make_prompt(args, testcase_files):
    def get_file_with_heading(filename, line_nums=False):
        return ("\n\n" +
            ("#" * 60) + "\n" +
            ("# File " + os.path.basename(filename) + "\n") +
            ("#" * 60) + "\n" +
            (add_line_nums if line_nums else lambda x: x)(read_whole_file(filename)))
    def remove_garbage_funcs(s):
        return re.sub(r'function (good|bad)\d+ \{\nBB entry:\n\s*I\d+:\s*ret void\s*// Line \d+\n\}\n*', '', s)
    unsat_core_file = os.getenv("OUT_DIR", "out") + "/" + "core.unsat.named"
    examined_core = subprocess.run(["/host/src/constraint_gen/examine_core.py", unsat_core_file, "--color-forced-lit", "none"],
        capture_output=True, text=True, check=False)
    ret = [
        read_whole_file(args.make_prompt),
        remove_garbage_funcs(get_file_with_heading(os.getenv("OUT_DIR", "out") + "/" + "numbered_ir.txt")),
        get_file_with_heading(unsat_core_file),
        "#"*80,
        "# Another view of the UNSAT core, using unit prop until a clause gets falsified" + examined_core.stdout]
    for c_file in sorted(testcase_files):
        ret.append(get_file_with_heading(c_file, line_nums=True))
    outf = (open(args.results_file, "w") if args.results_file else sys.stdout)
    outf.write("\n".join(ret) + "\n")
    if (outf != sys.stdout):
        outf.close()

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    default_listfile = script_dir + "/" + "juliet-temporal-testfiles.txt"
    parser = argparse.ArgumentParser(description='Process Juliet test cases (including multi-file test cases)',
        epilog="The environment variable OUT_DIR controls where temporary files are written.")
    parser.add_argument('--juliet_file_list', default=default_listfile, help=f'File that lists all Juliet files (default: {default_listfile})')
    parser.add_argument('-o', '--results-file', help='File to which to write results')
    parser.add_argument('--testcases-dir', default=script_dir, help='Directory containing the Juliet testcases')
    parser.add_argument('--testcase', nargs="+", help='Run only the specified testcase(s) (format: "CWE416,,malloc_free_char_01" or "CWE401,s01,char_calloc_01")')
    parser.add_argument('--config', choices=['GOOD', 'BAD'], help='Run only GOOD or BAD half')
    parser.add_argument('--unsat-core', action='store_true', help='Generate UNSAT core')
    parser.add_argument('--list-testcase-files', action='store_true', help='List the files of the specified test case')
    parser.add_argument('--make-prompt', type=str, help='Appends the contents of the C source code and $OUT_DIR/numbered_ir.txt and the UNSAT core to the specified prompt file and writes the result to results-file (if specified) or stdout.')
    parser.add_argument('--all-checks', action='store_true', help='Perform all checks (by default null-deref and mem-leak are disabled except for their CWE test cases)')
    parser.add_argument('--only-multifile', action='store_true', help='Only process multi-file test cases')
    parser.add_argument('--only-single-file', action='store_true', help='Only process single-file test cases')
    parser.add_argument('--quit-after', type=int, help='Only process the first N test cases (for debugging)')
    parser.add_argument('--start-at', type=str, help='Start at specified test case')
    parser.add_argument('--error-log', type=str, help='Write errors to log')
    parser.add_argument('--random-order', action="store_true", help='Orders by sha256(testname_name)')
    args = parser.parse_args()

    global cmdline_args
    cmdline_args = args

    if not os.getenv("COMPILER_OPTS"):
        sys.stderr.write("Warning: no COMPILER_OPTS specified!\n")

    if not os.getenv("POM_FILE"):
        sys.stderr.write("Warning: no POM_FILE specified!\n")

        if len(sys.argv) <= 1:
            print(parser.format_usage())
            print('Example usage: COMPILER_OPTS="-I/host/src/test/juliet" POM_FILE="../test.famous.llm.pom.yml" ./run_with_conv_dimacs_coproc.sh ./process_juliet_testsets.py  --only-multifile')
            return

    if args.unsat_core and not args.testcase:
        parser.error("--unsat-core requires --testcase")

    if args.make_prompt:
        args.unsat_core = True
        if not (args.testcase and args.config):
            parser.error("--make-prompt requires --testcase and --config")

    # Read file list
    with open(args.juliet_file_list, 'r') as f:
        filenames = f.readlines()

    sys.stderr.write("Collecting test cases...\n")
    testcase_to_files = collect_test_cases_from_file_list(filenames)
    sys.stderr.write(f"Found {len(testcase_to_files)} test cases (including C++ test cases)\n")

    # List testcase files if requested
    if args.list_testcase_files:
        for the_testcase in args.testcase:
            files = sorted(testcase_to_files[the_testcase])
            print(f"{the_testcase}: {files}")
        return

    # Running a specific testcase
    if args.testcase and len(args.testcase)==1:
        the_testcase = args.testcase[0]
        if the_testcase not in testcase_to_files:
            sys.stderr.write(f"Error: Testcase '{the_testcase}' not found\n")
            sys.exit(1)

        testcase_files = testcase_to_files[the_testcase]

        # Skip if any file is .cpp
        if any(f.endswith('.cpp') for f in testcase_files):
            sys.stderr.write(f"Skipping {the_testcase}: contains C++ files\n")
            sys.exit(0)

        if args.config:
            # Run just one half
            output = run_testcase_half(the_testcase, testcase_files, args.config, script_dir, args.unsat_core)
            if args.make_prompt:
                make_prompt(args, testcase_files)
                return
            else:
                print(output)
        else:
            # Run both halves
            output_good = run_testcase_half(the_testcase, testcase_files, 'GOOD', script_dir)
            output_bad = run_testcase_half(the_testcase, testcase_files, 'BAD', script_dir)
            print(output_good)
            print(output_bad)
    else:
        # Run all testcases
        results_file = args.results_file
        if not results_file:
            results_file = "results-" + datetime.now().strftime("%Y-%m-%d-%H%M") + ".txt"

        sys.stderr.write(f"Results will be written to: {results_file}\n")
        if args.error_log:
            error_log_filename = args.error_log
        else:
            error_log_filename = "errors-" + datetime.now().strftime("%Y-%m-%d-%H%M") + ".txt"
        global error_log_file
        error_log_file = open(error_log_filename, 'w')


        sys.stderr.write("Processing all test cases...\n")
        processed = 0
        skipped_c = 0
        skipped_cpp = 0
        time_start = time.time()

        hit_start = (False if args.start_at else True)

        with open(results_file, 'w') as outf:
            if args.testcase:
                testcase_list = args.testcase
            else:
                if args.random_order:
                    testcase_list = sorted(testcase_to_files.keys(), key=lambda name: hashlib.sha256(name.encode("utf-8")).hexdigest())
                else:
                    testcase_list = sorted(testcase_to_files.keys())

            for testcase_name in testcase_list:
                if testcase_name not in testcase_to_files:
                    sys.stderr.write(f"Error: Testcase '{testcase_name}' not found\n")
                    continue
                testcase_files = testcase_to_files[testcase_name]

                if testcase_name == args.start_at:
                    hit_start = True

                if not hit_start:
                    continue

                # Skip if any file is .cpp
                if any(f.endswith('.cpp') for f in testcase_files):
                    skipped_cpp += 1
                    continue

                if args.only_multifile and len(testcase_files) < 2:
                    skipped_c += 1
                    continue

                if args.only_single_file and len(testcase_files) > 1:
                    skipped_c += 1
                    continue

                if len(testcase_files) == 0:
                    sys.stderr.write(f"Error: Testcase '{testcase_name}' contains no files!\n")

                if not os.path.exists(sorted(testcase_files)[0]):
                    for err_out in [sys.stderr] + ([error_log_file] if error_log_file else []):
                        err_out.write(f"Skipping test case '{testcase_name}' because it contains a file that does not exist.\n")
                    continue

                sys.stderr.write(f"[{processed + 1}] Processing {testcase_name}...\n")


                if not args.config or args.config == "GOOD":
                    output_good = run_testcase_half(testcase_name, testcase_files, 'GOOD', script_dir)
                    outf.write(output_good + "\n")
                if not args.config or args.config == "BAD":
                    output_bad  = run_testcase_half(testcase_name, testcase_files, 'BAD',  script_dir)
                    outf.write(output_bad + "\n")
                outf.flush()

                processed += 1

                if args.quit_after and processed >= args.quit_after:
                    break

        time_elapsed = time.time() - time_start
        sys.stderr.write(f"\nProcessed {processed} test cases in {time_elapsed:.1f} sec, " +
            f"skipped {skipped_cpp} C++ test cases and {skipped_c} C test cases.\n")
        sys.stderr.write(f"Results written to {results_file}\n")


if __name__ == '__main__':
    main()
