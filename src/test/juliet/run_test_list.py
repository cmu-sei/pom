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
import pathlib
import subprocess
import yaml
from enum import Enum, auto
from dataclasses import dataclass

class TestResult(Enum):
    PASS = auto()
    FAIL = auto()
    INVALID = auto()

@dataclass
class TestInfo:
    basename: pathlib.Path
    goodness: str

TEST_TYPES = ["GOOD", "BAD"]

def short_name_to_test_info(short_name: str, testcases_path: pathlib.Path) -> TestInfo:
    fields = short_name.split(",")
    test_path = list(testcases_path.glob(f"{fields[0]}*"))[0]
    folder_name = test_path.name
    if fields[1]:
        test_path = test_path / fields[1]
    test_path = test_path / f"{folder_name}__{fields[2]}"
    return TestInfo(test_path, fields[3])


def run_test(c_files, test_type, juliet_dir, pom_file: str, function: str, library_pom, verbose: bool) -> TestResult:
    extra_llvm_opts = "-check-pom-complete"
    if pom_file.find("CWE401") == -1:
        extra_llvm_opts += " -no-mem-leak"
    if pom_file.find("CWE476") == -1:
        extra_llvm_opts += " -no-null-check"
    env = {
        "COMPILER_OPTS": f"-I{juliet_dir}/testcasesupport",
        "CLANG_VER": "15",
        "POM_FILE": f"{pom_file} {' '.join(library_pom)}",
        "FUNC": function,
        "LLVM_OPTS": extra_llvm_opts
    }
    command = [
        "/bin/bash",
        "run_juliet_test_half.sh",
        *[str(file) for file in c_files],
        test_type
    ]
    if verbose:
        command_line = " ".join([f"{key}='{value}'" for (key,value) in env.items()]) + " " + " ".join(command)
        print(f"[DEBUG] Running command: {command_line}")
    result = subprocess.run(command, env=env, capture_output=True)
    if verbose:
        print(f"[DEBUG] Stdout: {result.stdout.decode()}")
        print(f"[DEBUG] Stderr: {result.stderr.decode()}")
    if result.stdout.startswith("   pass".encode()):
        return TestResult.PASS
    elif result.stdout.startswith("!! FAIL".encode()):
        return TestResult.FAIL
    else:
        return TestResult.INVALID

def get_functions_from_pom_file(pom_file: pathlib.Path) -> list[str]:
    with open(pom_file) as file:
        pom = yaml.safe_load(file)

    return pom["Functions"].keys()

parser = argparse.ArgumentParser()
parser.add_argument("--list", "-l", type=pathlib.Path, help="File containing a list of tests to run")
parser.add_argument("--juliet-dir", "-j", type=pathlib.Path, help="Top-level Juliet directory")
parser.add_argument("--verbose", "-v", action="store_true")
parser.add_argument("--library-pom", type=str, action="append")
args = parser.parse_args()

tests = []
testcases_path = args.juliet_dir / "testcases"
with open(args.list) as file:
    for line in file:
        tests.append(short_name_to_test_info(line.strip(), testcases_path))

test_count = 0
all_success_count = 0
fail_count = 0
good_fail_count = 0
bad_fail_count = 0
skipped_count = 0
invalid_count = 0
failed_tests = []

for test in tests:
    print(f"---------- {test.basename} ({test.goodness})")
    test_dir = test.basename.parent
    c_files = list(test_dir.rglob(f"{test.basename.name}*.c"))
    pom_file = test.basename.with_suffix(".pom.yml")
    if not pom_file.exists():
        print(f"[WARNING] {pom_file} does not exist; skipping this test")
        skipped_count += 1
        continue
    functions = get_functions_from_pom_file(pom_file)

    all_success = True
    any_success = False
    for function in functions:

        if function.find(test.goodness.lower()) != -1:
            result = run_test(c_files, test.goodness, args.juliet_dir, str(pom_file), function, args.library_pom, args.verbose)
            if result == TestResult.PASS:
                any_success = True
            if result == TestResult.FAIL:
                all_success = False
            print(f"[RESULT] {function}: {result.name}")

    test_count += 1
    if test.goodness == "GOOD":
        if all_success:
            all_success_count += 1
        else:
            fail_count += 1
            failed_tests.append(test)
            good_fail_count += 1
    elif test.goodness == "BAD":
        if any_success:
            all_success_count += 1
        else:
            fail_count += 1
            failed_tests.append(test)
            bad_fail_count += 1

print()
print("Summary:")
print(f"    Tests run: {test_count}")
print(f"    Overall success: {all_success_count}")
print(f"    Overall failure: {fail_count}")
print(f"        BAD failures: {bad_fail_count}")
print(f"        GOOD failures: {good_fail_count}")
print(f"    Skipped: {skipped_count}")
print()
print("Failed tests:")
for test in failed_tests:
    print(f"    {test.basename} ({test.goodness})")
