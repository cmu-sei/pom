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


# Check if at least one argument is provided
USAGE="Usage: $0 <input.c> [-o <output.ll>] [-c compiler_opts]"
if [ $# -lt 1 ]; then
    echo $USAGE
    exit 1
fi

input_file="$1"
shift

# Initialize variables
output_file=""
compiler_opts=""

# Parse remaining arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -o)
            if [ $# -lt 2 ]; then
                echo "Error: -o requires an argument"
                echo $USAGE
                exit 1
            fi
            output_file="$2"
            shift 2
            ;;
        -c)
            shift
            # Everything after -c is compiler options
            compiler_opts="$@"
            break
            ;;
        *)
            echo "Error: Unknown option '$1'"
            echo $USAGE
            exit 1
            ;;
    esac
done

# If no output file specified, derive from input
if [ -z "$output_file" ]; then
    output_file="${input_file%.c}.raw.ll"
fi

# Run clang command
if [ "$CLANG_VER" == "18" ]; then
    clang-18 -ggdb -O0 -Xclang -fexperimental-assignment-tracking=enabled -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names -Xclang -disable-llvm-passes "$input_file" -o "$output_file" $compiler_opts
else
    clang-15 -ggdb -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names "$input_file" -o "$output_file" $compiler_opts
fi
