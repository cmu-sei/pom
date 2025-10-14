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

set -e
SCRIPT_DIR="$( dirname "${BASH_SOURCE[0]}" )"

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <input.c> <out_dir>"
  exit 1
fi

C_FILE=$1
OUT_DIR=$2
LL_FILE="${C_FILE%.c}.raw.ll"

$SCRIPT_DIR/compile_to_ll.sh $C_FILE
rm -f $OUT_DIR/solution.json
LLVM_OPTS="-glo-uniq $LLVM_OPTS" FUNC="__all_funcs" $SCRIPT_DIR/run.sh $LL_FILE $OUT_DIR
if [ ! -f "$OUT_DIR/solution.json" ]; then
    exit 1
fi;

POM_FILE="${C_FILE%.c}.pom.yml"
POM_FILE="$OUT_DIR/$(basename $POM_FILE)"
$SCRIPT_DIR/props_to_pom_yaml.py $OUT_DIR/solution.json -o $POM_FILE
echo "POM model written to: $POM_FILE"
