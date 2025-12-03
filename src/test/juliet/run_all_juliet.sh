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

start_time=$(date)
timestamp=$(date +"%Y-%m-%d-%H%M")

if [[ "$COMPILER_OPTS" == "" ]]; then
    export COMPILER_OPTS="-I/host/src/test/juliet"
fi
if [[ ! -v POM_FILE ]]; then # With "-v", there is no "$" before the variable name.
    export POM_FILE="/host/src/test/test.famous.llm.pom.yml"
fi
if [[ "$LLVM_OPTS" == "" ]]; then
    export LLVM_OPTS="-no-warn-arg-names -no-warn-func-ptr"
fi

for cwe_dir in CWE*; do
    for subdir in $cwe_dir $cwe_dir/s[0-9][0-9]; do
        if [[ "$ALL_CHECKS" == "true" ]]; then
            EXTRA_OPTS=""
        elif [[ $cwe_dir == CWE401* ]]; then # CWE401 is memory leak.
            EXTRA_OPTS="-no-null-check"
        elif [[ $cwe_dir == CWE476* ]]; then # CWE476 is null dereference.
            EXTRA_OPTS="-no-mem-leak"
        else
            EXTRA_OPTS="-no-mem-leak -no-null-check"
        fi       
        export LLVM_OPTS="$LLVM_OPTS $EXTRA_OPTS"
        echo
        echo Processing $subdir now...
        time ./run_juliet_tests.sh $subdir/*[0-9].c >> results-${cwe_dir:0:6}-$timestamp.txt
    done
done
echo "Starting time: $start_time"
echo "Ending time:   $(date)"
