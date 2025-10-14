#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# build_famous_fn_pom.py

# For every function name input, builds a POM signature for that
# function, dealing with its arguments and return value.  This will
# only work if the function is "famous" so that ChatGPT can find out
# what its signature and documentation is.

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


import argparse
import gzip
import json
import logging
import os
import sys
import subprocess

from collections import OrderedDict
from pathlib import Path
from typing import Any, List, Union

import yaml

from util import *
from verify import *

verbose: bool = False

# Logging setup
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

def parse_args() -> argparse.Namespace:
    """Set up and parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Build a POM model file against a source dir and AST dir")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output")
    parser.add_argument("pom_file", type=str,
                        help="The POM file to create")
    parser.add_argument("-c", "--cache", type=str, default="/host/src/cache.famous",
                        help="Directory to read intermediate data")
    return parser.parse_args()


def update_pom_from_answers(cache: str) -> None:
    """Parse .reply files, detect errors"""
    poms = []
    for fn_path in Path(cache, "Functions").iterdir():
        fn = fn_path.name
        fn_pom = []
        for group in ("args", "return"):
            group_path = Path(cache, "Functions", fn, group + ".reply");
            if group_path.exists():
                with group_path.open('r', encoding='utf-8') as f:
                    try:
                        wisdom = json.load(f)
                        fn_pom.append([group, wisdom[group]])
                        if group == "args":
                            fn_pom.append(["signature", wisdom["signature"]])
                    except Exception as e:
                        logging.warning(f"JSON error on {f}: {e}", str(group_path))
                        continue
        poms.append([fn, dict(fn_pom)])
    return {"Functions": dict(poms)}


def main() -> None:
    args = parse_args()
    global verbose
    verbose = args.verbose

    pom = update_pom_from_answers(args.cache)
    with open(args.pom_file, 'w') as f:
        yaml.dump(pom, f, default_flow_style=False)


if __name__ == "__main__":
    main()
