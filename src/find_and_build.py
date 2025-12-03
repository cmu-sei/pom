#!/usr/bin/env python3
"""
find_and_build.py

Iterate over all .c files in SRC_DIR, find the corresponding
.raw.ll file that declares the source filename, and run build.py
with the required arguments.
"""
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

import os
import argparse
import pathlib
import re
import subprocess
import sys
import shutil
import json
from dataclasses import dataclass

@dataclass
class TestInfo:
    basename: pathlib.Path
    goodness: str

def short_name_to_test_info(short_name: str, testcases_path: pathlib.Path) -> TestInfo:
    fields = short_name.split(",")
    test_path = list(testcases_path.glob(f"{fields[0]}*"))[0]
    folder_name = test_path.name
    if fields[1]:
        test_path = test_path / fields[1]
    test_path = test_path / f"{folder_name}__{fields[2]}"
    return TestInfo(test_path, fields[3])

# --------------------------------------------------------------------------- #
# Helper functions
# --------------------------------------------------------------------------- #

def find_matching_ll(c_file: pathlib.Path, ir_dir: pathlib.Path) -> pathlib.Path | None:
    """
    Search IR_DIR for a .raw.ll file that contains the line
        source_filename = "c_file.name"
    Return the Path to the .raw.ll file or None if not found.
    """
    # Compile the regex once
    pattern = re.compile(rf'source_filename\s*=\s*"({re.escape(c_file.name)})"')

    for ll_path in ir_dir.rglob("*.raw.ll"):
        try:
            with ll_path.open(encoding="utf-8") as fh:
                for line in fh:
                    if pattern.search(line):
                        return ll_path
        except (OSError, UnicodeDecodeError) as exc:
            print(f"[WARN] Skipping unreadable file {ll_path}: {exc}", file=sys.stderr)
    return None


def run_build(pom_yml: pathlib.Path, c_dir: pathlib.Path, asts: list[pathlib.Path]) -> int:
    """
    Execute the external build.py script with the three arguments.
    Returns the subprocess return code.
    """
    cmd = [
        sys.executable,  # use the same interpreter that is running this script
        "/host/src/build.py",
        str(pom_yml),
        str(c_dir),
        *[str(ast_gz) for ast_gz in asts]
    ]
    print(f"[INFO] Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=False)
    return result.returncode


# --------------------------------------------------------------------------- #
# Main logic
# --------------------------------------------------------------------------- #

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Find matching .raw.ll files for C sources and invoke build.py."
    )
    parser.add_argument(
        "src_dir",
        type=pathlib.Path,
        help="Directory that contains the .c source files."
    )
    parser.add_argument(
        "ir_dir",
        type=pathlib.Path,
        help="Directory that contains the .raw.ll files."
    )
    parser.add_argument(
        "build_database",
        type=pathlib.Path,
        help="Location of the build_database.json file to map source files to output files"
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Print detailed information."
    )
    parser.add_argument(
        "-l",
        "--list",
        type=pathlib.Path,
        help="Use a list of files to process"
    )
    parser.add_argument(
        "-o",
        "--overwrite",
        action="store_true",
        help="Overwrite existing POM files"
    )
    args = parser.parse_args()

    src_dir: pathlib.Path = args.src_dir.resolve()
    ir_dir: pathlib.Path  = args.ir_dir.resolve()

    if not src_dir.is_dir():
        sys.exit(f"[ERROR] Source directory does not exist: {src_dir}")
    if not ir_dir.is_dir():
        sys.exit(f"[ERROR] IR directory does not exist: {ir_dir}")

    with open(args.build_database) as build_database_file:
        build_database = json.loads(build_database_file.read())

    with open(args.list) as file:
        tests = [short_name_to_test_info(line.strip(), src_dir) for line in file]

    for test in tests:
        pom_yml = test.basename.with_suffix(".pom.yml")
        if pom_yml.exists():
            if not args.overwrite:
                if args.verbose:
                    print(f"\n[DEBUG] Skipping {test}, as a POM already exists for it")
                continue

        test_dir = test.basename.parent
        test_name = test.basename.name
        c_files = list(test_dir.rglob(f"{test_name}*.c"))

        asts = []
        for c_file in c_files:

            if c_file.name not in build_database:
                if args.verbose:
                    print(f"\n[DEBUG] Skipping file {c_file}, as it does not have an entry in the build database")
                continue

            if args.verbose:
                print(f"\n[DEBUG] Processing {c_file}")

            ll_path = pathlib.Path(ir_dir, build_database[c_file.name]["ll"])
            if ll_path is None:
                print(f"[WARN] No matching .raw.ll found for {c_file.name}")
                continue

            asts.append(pathlib.Path(ir_dir, build_database[c_file.name]["ast"]))

        # Run the external script
        if os.path.isdir("/host/src/cache"):
            shutil.rmtree("/host/src/cache")
        rc = run_build(pom_yml, test_dir, asts)
        if rc != 0:
            print(f"[ERROR] build.py failed for {test} (return code {rc})", file=sys.stderr)
        else:
            print(f"[SUCCESS] build.py succeeded for {test}")

        # Validate the POM file it produced


if __name__ == "__main__":
    main()
