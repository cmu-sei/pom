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

import asyncio
import aiofiles                # pip install aiofiles
from openai import AsyncOpenAI # pip install --upgrade openai

from util import *
from verify import *
from ask_gpt import run_tasks

verbose: bool = False

# Logging setup
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

def parse_args() -> argparse.Namespace:
    """Set up and parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Build a POM model file against a source dir and AST dir")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output")
    parser.add_argument("function_names", type=str, nargs="+",
                        help="Names of functions to obtain info about")
    parser.add_argument("-c", "--cache", type=str, default="/host/src/cache.famous",
                        help="Directory to store intermediate data")
    return parser.parse_args()


def cache_prompts(cache: str, fns: list[str]) -> None:
    """Create prompt questions for all functions"""
    prompt_dir = Path("/host/src")
    with Path(prompt_dir, "prompt.famous.args.md").open('r', encoding='utf-8') as f:
        args_prompt = f.read()
    with Path(prompt_dir, "prompt.famous.return.md").open('r', encoding='utf-8') as f:
        return_prompt = f.read()

    for fn in fns:
        fn_dir = Path(cache, "Functions", fn)
        fn_dir.mkdir(parents=True, exist_ok=True)

        env = {"function_name": fn}
        for (prompt, query_file) in [(args_prompt, "args.query"),
                                     (return_prompt, "return.query")]:
            fn_prompt = prompt.format(**env)
            with Path(fn_dir, query_file).open('w', encoding='utf-8') as f:
                f.write(fn_prompt)


def find_files(start_directory: str, suffix: str) -> List[Path]:
    """Finds all files that end with suffix in start_directory or a subdirectory"""
    suffix_files = []
    for root, _, files in os.walk(start_directory):
        for file in files:
            if file.endswith(suffix):
                suffix_files.append(Path(root, file))
    return suffix_files

async def get_llm_answers(cache: str) -> None:
    """Have an LLM provide a .reply file for each .query file that lacks one"""
    query_files = find_files(cache, ".query")
    if len(query_files) == 0:
        print(f"No .query files found in directory '{cache}'.")
        return
    client = AsyncOpenAI()  # respects OPENAI_API_KEY env var
    await run_tasks(query_files, client)

def update_pom_from_answers(cache: str, fns: list[str]) -> None:
    """Parse .reply files, detect errors"""
    for fn in fns:
        for group in ("args", "return"):
            group_path = Path(cache, "Functions", fn, group + ".reply");
            if group_path.exists():
                with group_path.open('r', encoding='utf-8') as f:
                    try:
                        wisdom = json.load(f)
                    except Exception as e:
                        logging.warning(f"JSON error on {f}: {e}", str(group_path))
                        continue


async def main() -> None:
    args = parse_args()
    global verbose
    verbose = args.verbose

    cache_prompts(args.cache, args.function_names)
    await get_llm_answers(args.cache)
    update_pom_from_answers(args.cache, args.function_names)


if __name__ == "__main__":
    asyncio.run(main())
