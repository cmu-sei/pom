#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# build.py
# Builds a Pointer Ownership Model (POM) file against a clang AST file and source directory

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
import json
import logging
import os
import sys
import subprocess

from pathlib import Path
from typing import List, Union

import asyncio
from openai import AsyncOpenAI # pip install --upgrade openai

import yaml
import yamale

from util import *
from verify import *
from ask_gpt import run_tasks

verbose: bool = False

POM_SCHEMA_FILE = os.environ.get("POM_SCHEMA_FILE", "/host/src/pom.yamale.yml")
POM_SCHEMA = yamale.make_schema(POM_SCHEMA_FILE)
INVALID_RETRIES = 5

# Logging setup
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)


def validate_pom_file(pom_file_path: str) -> bool:
    """
    Validate that the POM file passed in complies with the POM schema.

    :param pom_file_path: Filename of the POM file to validate
    :returns: True if the file exists is valid; False otherwise
    """
    pom_data = yamale.make_data(pom_file_path)
    if (
        not pom_data
        or not pom_data[0]
        or not isinstance(pom_data[0], tuple)
        or not pom_data[0][0]
    ):
        logger.error(f"POM file '{pom_file_path}' is empty or malformed.")
        return False
    try:
        yamale.validate(POM_SCHEMA, pom_data)
        logger.info(f"POM file '{pom_file_path}' validated (for syntactic correctness) successfully.")
        return True
    except yamale.YamaleError as e:
        logger.error(f"POM file '{pom_file_path}' failed validation.")
        for result in e.results:
            logger.warning(f"Validation error: {result}")
        return False


def parse_args() -> argparse.Namespace:
    """Set up and parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Build a POM model file against a source dir and AST dir")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output")
    parser.add_argument("pom_file", type=str,
                        help="The POM file to create")
    parser.add_argument("source_path", type=str,
                        help="Path to the directory with source files(s)")
    parser.add_argument("clang_ast_files", type=str, nargs="+",
                        help="Path(s) to the clang AST JSON (or GZ) file(s)")
    parser.add_argument("-c", "--cache", type=str, default="/host/src/cache",
                        help="Directory to store intermediate data")
    # parser.add_argument("pom_file", type=str, help="Path to the POM YAML file")
    # parser.add_argument("source_path", type=str, nargs="+",
    #                     help="Path(s) to the source directory")
    return parser.parse_args()


# Type for expressing the bounds of a function
# From redemption/extract_acr_func_bounds.py
type FuncBoundsType = tuple[str, int, int, str]
# [filename, begin_lin, end_line, function_name]

# Type for expressing POM Models
type POMType = Union[str, dict[str, POMType]]

func_bounds : list[FuncBoundsType] = []
current_file : Optional[Path] = None

class FuncVisitor(AstVisitor):
    def previsit(self, node: ASTNode) -> None:
        if not isinstance(node, dict):
            return
        func_like_decls = [
            "CXXMethodDecl",
            "CXXConstructorDecl",
            "CXXDestructorDecl",
            "CXXConversionDecl",
        ]
        if node.get("kind") in func_like_decls:
            self.visit_FunctionDecl(node)

    def visit_FunctionDecl(self, node: ASTNode) -> None:
        if "inner" not in node.keys():
            return
        global current_file
        cf = get_dict_path(node, "loc", "includedFrom", "file")
        if cf is not None and \
           (cf.endswith(".cpp") or cf.endswith(".c")):
            current_file = cf

        filename = get_dict_path(node, "loc", "file") \
                or get_dict_path(node, "end", "file") \
                or current_file
        if not filename:
            return
        current_file = filename

        if verbose:
            log("filename is " + str(filename) + " for node " + str(node["name"]))

        # Quick and dirty way to exclude system header files.
        # For a more robust solution, take the base dir as a command-line
        # argument and test whether the file is within the base dir.
        if filename.startswith("/usr"):
            return

        inner = node.get("inner")
        if not inner:
            return

        def get_body(inner: ASTNode) -> Optional[ASTNode]:
            for body in inner:
                try:
                    body_kind = body["kind"]
                except:
                    continue
                if body_kind == "CompoundStmt":
                    return body
            return None
        body = get_body(inner)
        if body is None:
            return

        def get_line(subnode: ASTNode) -> Optional[str]:
            if not subnode:
                return None
            if subnode.get("expansionLoc"):
                subnode = subnode.get("expansionLoc")
            return subnode.get("line")
        begin_line = get_line(get_dict_path(node, "range", "begin"))
        if not begin_line:
            begin_line = get_line(get_dict_path(node, "loc"))
        end_line   = get_line(get_dict_path(node, "range", "end"))
        if (not begin_line) or (not end_line):
            return

        func_bounds.append([filename, int(begin_line), int(end_line), node["name"]])


def build_function_pom( ast: ASTNode, fn_names: List[str]) -> list[tuple[ str, POMType]]:
    """Create POM data with stuff we can easily infer from AST"""
    poms = []
    for ast_func in ASTNode.get_all_functions(ast):
        fn_name = ast_func.get_name()
        if fn_name not in fn_names:
            continue
        fn_pom = {}

        if verbose:
            log(f"checking AST functions in POM: {fn_name}")

        # Args
        arglist = []
        for ast_arg in ast_func.arguments:
           if is_ptr(ast_arg.get_type()):
                arglist.append([ast_arg.get_name(), {"resp": "Unknown"}])
        fn_pom["args"] = dict(arglist)

        locallist = []
        for ast_local in ast_func.locals:
            if is_ptr(ast_local.get_type()):
                locallist.append([ast_local.get_name(), {"resp": "Unknown"}])
        fn_pom["locals"] = dict(locallist)

        # Return Value
        fn_pom["return"] = {"resp": "Unknown"} if is_ptr(ast_func.return_type) else {}

        poms.append([fn_name, fn_pom])

    return poms


def traverse_ast_files(ast_files: list[str]) -> POMType:
    """Builds a POModel and a list of function boundaries from the ASTs"""
    fn_bounds = []
    fn_pom = []

    for ast_file in ast_files:
        global func_bounds
        global current_file
        current_file = None
        func_bounds = []
        try:
            ast = read_clang_ast_file(ast_file, verbose) # from verify
            if not ast:
                logging.warning(f"AST data from {ast_file} is empty or invalid.")
                sys.exit(1)
            digested_ast = digest(ast)
            if not digested_ast:
                logging.warning(f"AST data from {ast_file} was unable to be digested.")
                sys.exit(1)
            if verbose:
                log(f"Digested AST: {digested_ast}")
        except Exception as e:
            logging.warning(f"Failed to read AST file {ast_file}: {e}")
            continue

        FuncVisitor().visit(ast)
        fn_bounds = fn_bounds + func_bounds
        fn_names = [fnb[3] for fnb in func_bounds]
        fn_pom = fn_pom + build_function_pom(digested_ast, fn_names)

    return {"Functions": dict(fn_pom)}, fn_bounds


def add_line_nums(lines: list[str]) -> list[str]:  # of lines with 'Line N' added
    """
    Takes the full text of a file as a list of strings, and returns a new string
    where each line (unless it ends with a backslash) has " // Line N" appended,
    with N being the 1-based line number.
    """
    out_lines = []
    # splitlines(True) preserves the line-ending characters
    for idx, line in enumerate(lines, 1):
        # Separate the line content from its newline(s)
        # so we can re-attach them after appending our marker.
        if line.endswith("\r\n"):
            content, newline = line[:-2], "\r\n"
        elif line.endswith("\n") or line.endswith("\r"):
            content, newline = line[:-1], line[-1]
        else:
            content, newline = line, ""

        # If the content ends with a backslash, do not append the marker
        if content.endswith("\\") or len(content.strip()) == 0 or content.strip().startswith('#'):
            out_lines.append(content + newline)
        else:
            out_lines.append(f"{content} // Line {idx}{newline}")

    return out_lines


def cache_srcs(cache: str, func_bounds: list[FuncBoundsType], source_dir: str) -> None:
    """Add source files and all function definitions to cache dir"""
    src_dir = Path(cache, "Source")
    src_dir.mkdir(parents=True, exist_ok=True)
    srcs = {}
    for (filename, begin_line, end_line, fn_name) in func_bounds:
        filename = Path(filename).name
        if filename not in srcs.keys():
            with Path(source_dir, filename).open('r', encoding='utf-8') as f:
                src_data = f.readlines()
                srcs[filename] = add_line_nums(src_data)
            with Path(src_dir, filename).open('w', encoding='utf-8') as f:
                f.writelines(srcs[filename])

        begin = int(begin_line) - 1
        end = int(end_line)
        fn_def = srcs[filename][begin:end]
        fn_dir = Path(cache, "Functions", fn_name)
        fn_dir.mkdir(parents=True, exist_ok=True)
        with Path(fn_dir, "src.c").open('w', encoding='utf-8') as f:
            f.writelines(fn_def)


def cache_prompts(cache: str, func_bounds: list[FuncBoundsType]) -> None:
    """Create prompt questions for all functions"""
    prompt_dir = Path("/host/src")
    with Path(prompt_dir, "prompt.args.md").open('r', encoding='utf-8') as f:
        args_prompt = f.read()
    with Path(prompt_dir, "prompt.local.md").open('r', encoding='utf-8') as f:
        locals_prompt = f.read()
    with Path(prompt_dir, "prompt.return.md").open('r', encoding='utf-8') as f:
        return_prompt = f.read()

    for (filename, begin_line, end_line, fn_name) in func_bounds:
        fn_dir = Path(cache, "Functions", fn_name)
        with Path(fn_dir, "src.c").open('r', encoding='utf-8') as f:
            fn_def = f.read()
        env = {"source_code": fn_def,
               "file_name": filename}
        for (prompt, query_file) in [(args_prompt, "args.query"),
                                     (locals_prompt, "locals.query"),
                                     (return_prompt, "return.query")]:
            fn_prompt = prompt.format(**env)
            with Path(fn_dir, query_file).open('w', encoding='utf-8') as f:
                f.write(fn_prompt)


def filter_prompts(cache: str, pom: POMType) -> None:
    """Remove queries that have no answer (because no pointers are involved)"""
    for (fn_name, fn_data) in pom["Functions"].items():
       if not any(map( lambda arg_data: arg_data["resp"] == "Unknown", fn_data["args"].values())):
           Path(cache, "Functions", fn_name, "args.query").unlink()

       if not any(map( lambda local_data: local_data["resp"] == "Unknown", fn_data["locals"].values())):
           Path(cache, "Functions", fn_name, "locals.query").unlink()

       if len(fn_data["return"]) == 0:
           Path(cache, "Functions", fn_name, "return.query").unlink()


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

def update_pom_from_answers(cache: str, pom: POMType) -> None:
    """Update POM with LLM .reply files"""
    for (fn_name, fn_data) in pom["Functions"].items():
        for group in ("args", "locals", "return"):
            group_path = Path(cache, "Functions", fn_name, group + ".reply");
            if group_path.exists():
                with group_path.open('r', encoding='utf-8') as f:
                    try:
                        wisdom = json.load(f)
                        pom["Functions"][fn_name][group] = wisdom[group]
                    except Exception as e:
                        logging.warning(f"JSON error on {f}: {e}", str(group_path))
                        continue


async def main() -> None:
    args = parse_args()
    source_path = Path(args.source_path)
    if source_path.is_file():
        source_dir = source_path.parent
    elif source_path.is_dir():
        source_dir = source_path
    else:
        raise ValueError("Invalid source path: must be a file or directory")

    global verbose
    verbose = args.verbose

    pom, fn_bounds = traverse_ast_files(args.clang_ast_files)
    cache_srcs(args.cache, fn_bounds, source_dir)
    cache_prompts(args.cache, fn_bounds)
    filter_prompts(args.cache, pom)

    for try_num in range(INVALID_RETRIES):
        await get_llm_answers(args.cache)
        update_pom_from_answers(args.cache, pom)
        with open(args.pom_file, 'w') as f:
            yaml.dump(pom, f, default_flow_style=False)

        # Validate the POM file
        pom_file_is_valid = validate_pom_file(args.pom_file)
        if pom_file_is_valid:
            sys.exit(0)
        else:
            if try_num < INVALID_RETRIES - 1:
                logger.warning("Produced invalid POM file; will retry")
            else:
                logger.warning("Produced invalid POM file")

    logger.error("Too many retries; aborting")
    sys.exit(1)

if __name__ == "__main__":
    asyncio.run(main())
