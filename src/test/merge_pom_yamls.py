#!/usr/bin/env python3
"""
Merge two YAML files together, detecting and reporting conflicts.
"""
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
import yaml
import argparse
import re
from typing import Any, Dict, List

class QuotedString(str):
    pass

def quoted_string_representer(dumper, data):
    return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='"')

yaml.add_representer(QuotedString, quoted_string_representer)

def represent_list(dumper, data):
    # Use flow style (inline) for short lists with simple values
    if len(data) <= 3 and all(isinstance(item, (str, int, float, bool, type(None))) for item in data):
        return dumper.represent_sequence('tag:yaml.org,2002:seq', data, flow_style=True)
    return dumper.represent_sequence('tag:yaml.org,2002:seq', data, flow_style=False)

yaml.add_representer(list, represent_list)


def merge_dicts(dict1: Dict, dict2: Dict, path: List[str] = None) -> Dict:
    """
    Recursively merge dict2 into dict1.

    If a key path exists in both dictionaries, print an error to stderr
    and keep the value from dict1 (the first file).

    Args:
        dict1: First dictionary (takes precedence in conflicts)
        dict2: Second dictionary
        path: Current key path for error reporting

    Returns:
        Merged dictionary
    """
    if path is None:
        path = []

    result = dict1.copy()

    for key, value2 in dict2.items():
        current_path = path + [key]
        path_str = '.'.join(current_path)

        if key not in result:
            # Key doesn't exist in dict1, so add it
            result[key] = value2
        else:
            value1 = result[key]

            # Both dictionaries have this key
            if isinstance(value1, dict) and isinstance(value2, dict):
                # Both values are dictionaries, so merge recursively
                result[key] = merge_dicts(value1, value2, current_path)
            else:
                # Conflict: same key path with non-dict values (or one is dict, one isn't)
                print(f"Error: Conflict at key path '{path_str}'. Keeping value from first file.",
                      file=sys.stderr)
                # Keep value1 (from first file) as specified

    return result

def fixup_yaml(node):
    if isinstance(node, dict):
        for (key, value) in node.items():
            if key == "signature":
                value = QuotedString(value)
            node[key] = fixup_yaml(value)
        return node
    else:
        return node


def main():
    parser = argparse.ArgumentParser(
        description='Merge two YAML files together. '
                    'If the same key path appears in both files, '
                    'the value from the first file is kept and an error is printed to stderr.'
    )
    parser.add_argument('file1', help='First YAML file')
    parser.add_argument('file2', help='Second YAML file, or "{}" for empty dict')
    parser.add_argument('-o', '--output', help='Output file (default: stdout)', default=None)

    args = parser.parse_args()
    data = [None, None]

    # Load the YAML files
    for (idx, filename) in enumerate([args.file1, args.file2]):
        if filename == "{}":
            data[idx] = {}
            continue
        try:
            with open(filename, 'r') as f:
                data[idx] = yaml.safe_load(f)
            if data[idx] is None:
                data[idx] = {}
        except FileNotFoundError:
            print(f"Error: File '{filename}' not found.", file=sys.stderr)
            sys.exit(1)
        except yaml.YAMLError as e:
            print(f"Error: Failed to parse YAML file '{filename}': {e}", file=sys.stderr)
            sys.exit(1)

        if not isinstance(data[idx], dict):
            print(f"Error: '{filename}' must contain a dictionary at the root level.", file=sys.stderr)
            sys.exit(1)

    # Merge the dictionaries
    merged = merge_dicts(data[0], data[1])
    merged = fixup_yaml(merged)

    # Output the result
    output_yaml = yaml.dump(merged, default_flow_style=False, sort_keys=False, width=float("inf"))

    # Add a blank line before each function
    output_yaml = re.sub(r'(?m)(?=^  [^ ])', '\n', output_yaml)

    if args.output:
        try:
            with open(args.output, 'w') as f:
                f.write(output_yaml + "\n")
        except IOError as e:
            print(f"Error: Failed to write to output file '{args.output}': {e}", file=sys.stderr)
            sys.exit(1)
    else:
        print(output_yaml, end='')


if __name__ == '__main__':
    main()
