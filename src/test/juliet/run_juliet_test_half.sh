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

# Make sure the header files are accessible; set $COMPILER_OPTS if necessary to specify include dir.
# Produce io.raw.ll by running compile_to_ll.sh on io.c (in the testcasesupport directory).

if [ $# -lt 2 ]; then
    echo "Usage: $0 <testfile.c> [GOOD|BAD] [--unsat-core]"
    echo "The environment OUT_DIR controls where output files go (default: 'out' in current directory)."
    exit 1
fi

if [ "$OUT_DIR" == "" ]; then
    OUT_DIR="out"
fi

testfile=$1
define_config=$2
if [ "$define_config" == "GOOD" ]; then
    define_switch="-DOMITBAD"
    expected_result="SAT"
elif [ "$define_config" == "BAD" ]; then
    define_config="BAD "
    define_switch="-DOMITGOOD"
    expected_result="UNSAT"
else
    echo "Error: Unknown config '$define_config'!"
    exit 1
fi

set -e
/host/src/constraint_gen/compile_to_ll.sh $testfile -o test_alone.raw.ll -c $define_switch $COMPILER_OPTS
llvm-link-$CLANG_VER test_alone.raw.ll io.raw.ll -S -o test_linked.raw.ll
TEST_LL=test_linked.raw.ll
#TEST_LL=test_alone.raw.ll

LLVM_OPTS="-whole-prog -no-null-check $LLVM_OPTS" FUNC="__all_funcs" /host/src/constraint_gen/run.sh $TEST_LL -o $OUT_DIR > /dev/null
if [ "$3" == "--unsat-core" ]; then
    LLVM_OPTS="-whole-prog -no-null-check" FUNC="__all_funcs" /host/src/constraint_gen/get_unsat_core.sh $TEST_LL -o $OUT_DIR --collapse-flow
fi

actual_result=`head -n 1 $OUT_DIR/solution.txt`
if [ "$actual_result" == "$expected_result" ]; then
    pass_or_fail="   pass"
else
    pass_or_fail="!! FAIL"
fi
echo "$pass_or_fail: $define_config version of $testfile"
