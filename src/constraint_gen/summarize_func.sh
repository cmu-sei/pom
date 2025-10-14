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


if [ "$#" -ne 2 ]; then
  echo "Usage: [FUNC=<funcname>] [POM_FILE=<file.pom.yml>] $0 <input.ll> <output_dir>"
  echo "  <input.ll>     LLVM IR file to process"
  echo "  <output_dir>   Directory in which to write outputs"
  exit 1
fi

LL_FILE=$1  # The input ".ll" file
OUT_DIR=$2  # Directory in which to write output files

if [[ "$2" = "-o" ]]; then
    OUT_DIR=$3
fi

SCRIPT_DIR="$( dirname "${BASH_SOURCE[0]}" )"

set -e

FUNC_ARG=${FUNC:+--func $FUNC}
set -e
$SCRIPT_DIR/run.sh $LL_FILE $OUT_DIR --no-solve > /dev/null
set +e
python3 $SCRIPT_DIR/conv_dimacs.py $FUNC_ARG $OUT_DIR/constraints.txt -v $OUT_DIR/var_map.json -o $OUT_DIR/constraints.dimacs --summarize
rm -f $OUT_DIR/constraints.cqbf
python3 $SCRIPT_DIR/ghostq/qcir-conv-py3.py $OUT_DIR/constraints.dimacs --write-gq --keep-varnames --quiet -o $OUT_DIR/constraints.cqbf
$SCRIPT_DIR/ghostq/ghostq $OUT_DIR/constraints.cqbf -allow-free | $SCRIPT_DIR/ghostq/fmla -
