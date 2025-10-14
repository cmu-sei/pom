#!/usr/bin/env python3
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


import sys
import os
import re

def split_constraints(input_file, output_dir, quiet=False):
    try:
        # First pass: collect common lines (outside function blocks)
        common_lines = []

        if not quiet:
            sys.stdout.write("Finding common lines...\n")
        with open(input_file, 'r') as f:
            inside_function = False
            for line in f:
                line = line.rstrip('\n\r')  # Remove newlines but keep other whitespace

                if line.startswith('<function name="'):
                    inside_function = True
                elif line == '</function>':
                    inside_function = False
                elif not inside_function and line.strip():  # Non-empty line outside function
                    common_lines.append(line)

        # Create output directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # Second pass: create output files
        with open(input_file, 'r') as f:
            current_function = None
            output_file = None

            line_num = 0
            for line in f:
                line_num += 1
                line = line.rstrip('\n\r')

                if line.startswith('<function name="'):
                    # Extract function name
                    match = re.match(r'<function name="([^"]+)">', line)
                    if match:
                        current_function = match.group(1)
                        output_path = os.path.join(output_dir, f'constraints_{current_function}.txt')
                        if not quiet:
                            sys.stdout.write(f"Processing function {current_function}...\n")
                        output_file = open(output_path, 'w')
                        output_file.write(line + '\n')
                    else:
                        # Malformed function tag, skip
                        sys.stderr.write(f"Error parsing line {line_num}.\n")
                        current_function = None
                        output_file = None
                elif line == '</function>':
                    if current_function and output_file:
                        output_file.write(line + '\n')
                        # Write common lines
                        for common_line in common_lines:
                            output_file.write(common_line + '\n')
                        output_file.close()
                    output_file = None
                    current_function = None
                elif current_function and output_file:
                    output_file.write(line + '\n')

    except FileNotFoundError:
        print(f"Error: Input file '{input_file}' not found.")
        sys.exit(1)
    except IOError as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: ./split_constraints.py constraints.txt outdir")
        sys.exit(1)

    input_file = sys.argv[1]
    output_dir = sys.argv[2]

    split_constraints(input_file, output_dir)
