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

from collections import OrderedDict, defaultdict
import argparse, json, sys, os

def read_json_file(filename):
    with open(filename, 'rt') as f:
        try:
            data = json.load(f, object_pairs_hook=OrderedDict)
        except Exception as e:
            sys.stderr.write(
                "Error reading JSON file: {}: {}\n".format(filename, e))
            sys.exit(1)
    return data

def main():
    parser = argparse.ArgumentParser(description='Convert solution.json to props format')
    parser.add_argument('input', help='Input solution.json file')
    parser.add_argument('-o', '--output', help='Output props file (default: stdout)')
    args = parser.parse_args()

    soln = read_json_file(args.input)
    if args.output:
        out_file = open(args.output, 'w')
    else:
        out_file = sys.stdout
    for (prop, truth_val) in soln.items():
        assert(truth_val is True or truth_val is False)
        line = ("!" if truth_val==False else "") + prop
        out_file.write(line + "\n")

main()
