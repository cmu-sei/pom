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


if [ $# -lt 1 ]; then
    echo "Usage: $0 <input.ll> [-o out_dir] [--collapse-flow]"
    echo "If unspecified, out_dir defaults to ./out"
    exit 1
fi

# Look for optional arguments
collapse_flow=""
new_args=()
for arg in "$@"; do
  if [[ "$arg" == "--collapse-flow" ]]; then
    collapse_flow="--collapse-flow"
  else
    new_args+=("$arg")
  fi
done
# Replace the positional parameters with the filtered list
set -- "${new_args[@]}"

# Check if -o flag is provided
if [ $# -ge 2 ] ; then
    if [ "$2" = "-o" ]; then
        out_dir="$3"
    else
        echo "Error: 2nd argument expected to be '-o'."
        exit 1
    fi
else
    out_dir=out
fi

INPUT_LL=$1
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

drat_trim=$(find $SCRIPT_DIR -name drat-trim -and -type f -and -perm -u=x | head -n 1)

if [ "$drat_trim" = "" ]; then
    drat_trim=$(find /opt/drat-trim -name drat-trim -and -type f -and -perm -u=x | head -n 1)
fi

if [ "$drat_trim" = "" ]; then
    echo "Error: drat-trim executable not found!"
    exit 1
fi

rm -f out/constraints.{txt,dimacs}
set -e
bash $SCRIPT_DIR/run.sh $INPUT_LL $out_dir --no-solve
set +e
RAND_SEED_ARG=${RAND_SEED:+--random $RAND_SEED}
cryptominisat $out_dir/constraints.dimacs $out_dir/proof.drat $RAND_SEED_ARG --verb 0 > /dev/null
rm -f $out_dir/core.unsat $out_dir/core.unsat.named
$drat_trim $out_dir/constraints.dimacs $out_dir/proof.drat -c $out_dir/core.unsat > /dev/null
if [ ! -f "$out_dir/core.unsat" ]; then
    echo "No unsat core!"
    exit 1
fi
FUNC_ARG=${FUNC:+--func $FUNC}
python3 $SCRIPT_DIR/conv_dimacs.py $FUNC_ARG --from-dimacs $out_dir/core.unsat  -o $out_dir/core.unsat.named -v $out_dir/var_map.json --clause-info-file $out_dir/constraints.txt $collapse_flow --catch-exc
echo
cat $out_dir/core.unsat.named
