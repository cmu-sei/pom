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
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
SRC_DIR = TEST_DIR.parent


@pytest.mark.parametrize(
    "basename", [f.name.removesuffix(".c") for f in TEST_DIR.glob("*.c")]
)
def test_solver(basename):
    source_file = TEST_DIR / f"{basename}.c"
    pom_file = TEST_DIR / f"{basename}.pom.yml"
    ll_file = TEST_DIR / f"{basename}.raw.ll"
    expect_file = TEST_DIR / f"{basename}.expect.json"

    # Ensure that all the files we need are there
    assert source_file.is_file()

    # Run the verifier
    if expect_file.is_file() and pom_file.is_file() and ll_file.is_file():
        result = subprocess.run(
            f"python3 run_solver.py --json --pom-file {pom_file} --library-pom /host/src/test/test.famous.llm.pom.yml {ll_file}",
            shell=True,
            capture_output=True,
            text=True,
            cwd=SRC_DIR,
        )
        actual_output = json.loads(result.stdout)

        # Read in the expected result
        with open(expect_file) as expect_file_handle:
            expected_output = json.load(expect_file_handle)

        # Diff against the actual output
        assert actual_output == expected_output
    else:
        pytest.skip(f"Needed files not found for {basename}; skipping")
