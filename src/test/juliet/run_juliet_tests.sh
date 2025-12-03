#!/usr/bin/env bash
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

if [ "$OUT_DIR" == "" ]; then
    export OUT_DIR="out"
fi

if [ ! -z "$POM_FILE" ]; then
    export PROPS_FILE=$OUT_DIR/pom.juliet.lib.constraints.txt
    export ARG_NAMES_FILE=$OUT_DIR/juliet.lib.argnames.txt
    /host/src/constraint_gen/pom_yaml_to_props.py $POM_FILE --arg-name-file $ARG_NAMES_FILE > $PROPS_FILE
    unset POM_FILE
fi

export NO_SSA_FILE="true"
export SOL_JSON_ARG="--no-solution-json"


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

set -e
for testfile in $@; do
    $SCRIPT_DIR/run_juliet_test_half.sh $testfile GOOD
    $SCRIPT_DIR/run_juliet_test_half.sh $testfile BAD
done
