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
  echo "Usage: [FUNC=<funcname>] [POM_FILE=<file.pom.yml>] $0 <input.ll> <output_dir> [--no-solve]"
  echo "  <input.ll>     LLVM IR file to process"
  echo "  <output_dir>   Directory in which to write outputs"
  exit 1
fi

LL_FILE=$1  # The input ".ll" file
OUT_DIR=$2  # Directory in which to write output files

if [[ "$2" = "-o" ]]; then
    OUT_DIR=$3
fi

# Check that the input file exists
if [ ! -f "$LL_FILE" ]; then
  echo "Error: input file '$LL_FILE' not found."
  exit 1
fi

if [[ "$LL_FILE" == *.c ]]; then
  C_FILE=$LL_FILE
  LL_FILE="${C_FILE%.c}.raw.ll"
  $SCRIPT_DIR/compile_to_ll.sh $C_FILE -c $COMPILE_OPTS
fi

# Check that the input file has a .ll extension
if [[ ! "$LL_FILE" == *.ll ]]; then
  echo "Warning: input filename '$LL_FILE' does not end with '.ll'."
  echo "Input file must be LLVM IR, not C source code."
fi

# Create the output directory if it doesn't exist
if [ ! -d "$OUT_DIR" ]; then
  echo "Output directory '$OUT_DIR' does not exist. Creating it..."
  mkdir -p "$OUT_DIR" || { echo "Failed to create '$OUT_DIR'."; exit 1; }
fi
rm -f $OUT_DIR/constraints.txt

pushd $SCRIPT_DIR > /dev/null
make
popd > /dev/null

if [[ $LL_FILE == *.raw.ll ]]; then
    LL_SSA_FILE="$OUT_DIR/$(basename "${LL_FILE%.raw.ll}.ssa.ll")"
elif [[ $LL_FILE == *.ll ]]; then
    LL_SSA_FILE="$OUT_DIR/$(basename "${LL_FILE%.ll}.ssa.ll")"
fi

FUNC_ARG=${FUNC:+--func $FUNC}
POM_PROPS_ARG=${POM_FILE:+-pom-props $OUT_DIR/pom.constraints.txt -arg-name-file $OUT_DIR/argnames.txt}
if [ ! -z "$POM_FILE" ]; then
    $SCRIPT_DIR/pom_yaml_to_props.py $POM_FILE --arg-name-file $OUT_DIR/argnames.txt > $OUT_DIR/pom.constraints.txt
fi
if [ "$CLANG_VER" == "18" ]; then
    opt-$CLANG_VER -load-pass-plugin $SCRIPT_DIR/VarStorePass.so -passes=var-store,mem2reg $LL_FILE -S -o $LL_SSA_FILE
    opt-$CLANG_VER -load-pass-plugin $SCRIPT_DIR/ConstraintGenPass.so -passes=constraint-gen -output $OUT_DIR/constraints.txt $LLVM_OPTS -numir $OUT_DIR/numbered_ir.txt $LL_SSA_FILE $POM_PROPS_ARG -S -o /dev/null
else
    opt-15 -enable-new-pm=0 -load $SCRIPT_DIR/VarStorePass.so -var-store -mem2reg $LL_FILE -S -o $LL_SSA_FILE
    opt-15 -enable-new-pm=0 -load $SCRIPT_DIR/ConstraintGenPass.so -constraint-gen -output $OUT_DIR/constraints.txt $LLVM_OPTS -numir $OUT_DIR/numbered_ir.txt $LL_SSA_FILE $POM_PROPS_ARG -S -o /dev/null
fi
if [ ! -z "$POM_FILE" ]; then
    $SCRIPT_DIR/pom_yaml_to_props.py $POM_FILE >> $OUT_DIR/constraints.txt
fi
python3 $SCRIPT_DIR/conv_dimacs.py $FUNC_ARG $OUT_DIR/constraints.txt -v $OUT_DIR/var_map.json -o $OUT_DIR/constraints.dimacs

if [ "$3" != "--no-solve" ]; then
    set +e
    RAND_SEED_ARG=${RAND_SEED:+-rnd-seed=$RAND_SEED}
    minisat $OUT_DIR/constraints.dimacs $RAND_SEED_ARG $OUT_DIR/solution.txt
    first_line=$(head -n 1 "$OUT_DIR/solution.txt")
    if [ "$first_line" == "SAT" ]; then
        python3 $SCRIPT_DIR/conv_dimacs.py $OUT_DIR/solution.txt -v $OUT_DIR/var_map.json -o $OUT_DIR/solution.json --from-sol
        echo "Solution written to $OUT_DIR/solution.json"
    elif [ "$first_line" == "UNSAT" ]; then
        echo "Unsatisfiable!"
    else
        echo "Error: the first line of solution.txt ('$first_line') is not 'SAT' or 'UNSAT'."
    fi
fi
