#!/bin/bash
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

# Runs a single half (GOOD or BAD) of a Juliet test case that may consist of multiple files.
# Make sure the header files are accessible; set $COMPILER_OPTS if necessary to specify include dir.
# Make sure that io.raw.ll exists in the current directory (produce it by
# running compile_to_ll.sh on io.c, found in the testcasesupport directory).


if [ $# -lt 2 ]; then
    echo "Usage: $0 <file1.c> [file2.c ...] [GOOD|BAD] [--unsat-core]"
    echo "The environment variable OUT_DIR controls where output files go (default: 'out' in current directory)."
    echo "The environment variable TESTCASE_NAME can be set to specify the test case name for output."
    exit 1
fi

if [ "$OUT_DIR" == "" ]; then
    OUT_DIR="out"
fi

# Parse arguments - last non-option argument is the config (GOOD or BAD)
# All arguments before that are files
files=()
config=""
unsat_core_flag=""

for arg in "$@"; do
    if [ "$arg" == "--unsat-core" ]; then
        unsat_core_flag="$arg"
    elif [ "$arg" == "GOOD" ] || [ "$arg" == "BAD" ]; then
        config="$arg"
    else
        files+=("$arg")
    fi
done

if [ "$config" == "" ]; then
    echo "Error: Must specify GOOD or BAD"
    exit 1
fi

if [ ${#files[@]} -eq 0 ]; then
    echo "Error: No input files specified"
    exit 1
fi

define_config="$config"
if [ "$config" == "GOOD" ]; then
    define_switch="-DOMITBAD"
    expected_result="SAT"
elif [ "$config" == "BAD" ]; then
    define_config="BAD " # Extra space for alignment
    define_switch="-DOMITGOOD"
    expected_result="UNSAT"
else
    echo "Error: Unknown config '$config'!"
    exit 1
fi

set -e

# Remove any leftovers from previous runs
rm -f $OUT_DIR/temp_input_*.raw.ll

# Compile all .c files to .ll files
ll_files=()
counter=1
for testfile in "${files[@]}"; do
    if [[ "$testfile" == *.c ]]; then
        ll_file="$OUT_DIR/temp_input_${counter}.raw.ll"
        /host/src/constraint_gen/compile_to_ll.sh "$testfile" -o "$ll_file" -c $define_switch $COMPILER_OPTS
        ll_files+=("$ll_file")
        ((counter++))
    fi
done


if [ ${#ll_files[@]} -eq 0 ]; then
    echo "Error: No .c files to compile"
    exit 1
fi

# Link all .ll files together with io.raw.ll
ll_files_to_link="${ll_files[@]} io.raw.ll"
set +e
ls $ll_files_to_link > /dev/null # Generate a warning about missing files
set -e
llvm-link-$CLANG_VER $ll_files_to_link -S -o $OUT_DIR/test_linked.raw.ll

TEST_LL=$OUT_DIR/test_linked.raw.ll

# Run the analysis
export HAVE_WHOLE_PROG="true"
if [ "$FUNC" != "" ]; then
    LLVM_OPTS="-whole-prog $LLVM_OPTS" FUNC="__all_funcs" /host/src/constraint_gen/run.sh $TEST_LL $OUT_DIR $SOL_JSON_ARG > /dev/null
else
    /host/src/constraint_gen/run.sh $TEST_LL $OUT_DIR $SOL_JSON_ARG > /dev/null
fi
actual_result=$(head -n 1 $OUT_DIR/solution.txt)

# Generate UNSAT core if requested
if [ "$unsat_core_flag" == "--unsat-core" ]; then
    LLVM_OPTS="-whole-prog $LLVM_OPTS" FUNC="__all_funcs" /host/src/constraint_gen/get_unsat_core.sh $TEST_LL -o $OUT_DIR --collapse-flow
fi

# Determine pass/fail
if [ "$actual_result" == "$expected_result" ]; then
    pass_or_fail="   pass"
else
    pass_or_fail="!! FAIL"
fi

# Extract unsupported features
unsupported=$(grep "^# UNSUPPORTED_FEATURES:" $OUT_DIR/constraints.txt 2>/dev/null | sed 's/_FEATURES//g' || echo "")

# Collect staticReturns from all files
staticReturns=""
has_ptr_ptr_cast=""
set +e
for testfile in "${files[@]}"; do
    if [[ "$testfile" == *.c ]]; then
        file_returns=$(grep -o "staticReturns[A-Za-z0-9_]*" "$testfile" 2>/dev/null | sort | uniq || true)
        if [ ! -z "$file_returns" ]; then
            staticReturns="$staticReturns $file_returns"
        fi
        file_ptr_ptr_cast=$(egrep '[(][^)]*[*] ?[*][)]' $testfile)
        if [ ! -z "$file_ptr_ptr_cast" ]; then
            has_ptr_ptr_cast=" [CAST_TO_PTR_PTR]"
        fi
    fi
done

# Remove duplicates and clean up
staticReturns=$(echo "$staticReturns" | tr ' ' '\n' | sort | uniq | tr '\n' ' ')

# Use TESTCASE_NAME if set, otherwise use first file
testname="${TESTCASE_NAME:-${files[0]}}"

## Clean up intermediate .ll files
#for ll_file in "${ll_files[@]}"; do
#    rm -f "$ll_file"
#done

echo "$pass_or_fail: $define_config version of $testname$SINGLE_OR_MULTI $unsupported$has_ptr_ptr_cast" $staticReturns
