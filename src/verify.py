#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# verify.py
# Verifies a Pointer Ownership Model (POM) file against a clang AST file.
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
import difflib
import gzip
import json
import logging
import os
import sys

from collections import OrderedDict
from pathlib import Path
from typing import OrderedDict, Tuple, Optional

import yamale

from digest import *

# Logging setup
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

# Load schema file from environment variable
POM_SCHEMA_FILE = os.environ.get("POM_SCHEMA_FILE", "/opt/pom/pom.yamale.yml")
POM_SCHEMA = yamale.make_schema(POM_SCHEMA_FILE)


def log(obj):
    logging.info(obj)


def warn(obj):
    logging.warning(obj)


def error(obj):
    logging.error(obj)


def diff(s1: str, s2: str) -> None:
    lines_s1 = s1.splitlines()
    lines_s2 = s2.splitlines()

    diff = difflib.unified_diff(lines_s1, lines_s2, lineterm="")
    for line in diff:
        if line.startswith('-'):
            print(f"\033[31m{line}\033[0m")  # Red for removals
        elif line.startswith("+"):
            print(f"\033[32m{line}\033[0m")  # Green for additions
        else:
            print(line)


def parse_args():
    """Set up and parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Verify a POM file against a clang AST file.")
    parser.add_argument("pom_file", type=str, help="Path to the POM YAML file")
    parser.add_argument("clang_ast_file", type=str,
                        help="Path to the clang AST JSON (or GZ) file(s)")
    parser.add_argument(
        "--output", type=str, help="Output verification results in a structured format")
    parser.add_argument("--validate", action="store_true",
                        help="Validate the POM file against the schema")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output")

    return parser.parse_args()


def is_ptr(type):
    """ Returns if type indicates a pointer. """
    # TODO: Use nicholas's enhanced type feature from Clang ASTs.
    return type is not None and "*" in type


class PointerOwnershipModel:

    raw_data: OrderedDict
    functions: OrderedDict[str, OrderedDict]
    file_path: str

    def __init__(self, yaml: dict):
        """
        Initialize a Pointer Ownership Model from a loaded YAML file.
        """

        self.raw_data = OrderedDict(yaml)
        self.functions = OrderedDict()

        raw_functions: OrderedDict = yaml["Functions"]

        for function in raw_functions:
            function_name = function

            """
            Structure of pom_function_data
            args: OrderedDict
            locals: OrderedDict
            return: OrderedDict
            """
            pom_function_data: OrderedDict = raw_functions[function]

            self.functions[function_name] = pom_function_data

    def get_function(self, function_name: str):
        return self.functions[function_name]

    def get_functions(self):
        return self.functions

    def get_function_names(self):
        return list(self.functions.keys())

    def set_file_path(self, file_path: str) -> None:
        self.file_path = file_path

    @staticmethod
    def read_from_file(pom_file_path: str, validate=True, verbose=False) -> 'PointerOwnershipModel':
        if verbose:
            log(
                f"PointerOwnershipModel.read_from_file({pom_file_path}, validate={validate})")

        pom_data = yamale.make_data(pom_file_path)

        if not pom_data or not pom_data[0] or not isinstance(pom_data[0], tuple) or not pom_data[0][0]:
            warn(f"POM file '{pom_file_path}' is empty or malformed.")

            sys.exit(1)

        if validate:
            try:
                yamale.validate(POM_SCHEMA, pom_data)

                if verbose:
                    log(f"POM file '{pom_file_path}' validated successfully.")
            except yamale.YamaleError as e:
                warn(f"POM file '{pom_file_path}' failed validation.")

                for result in e.results:
                    warn(f"Validation error: {result}")

                sys.exit(1)

        raw_pom_data = pom_data[0][0]

        pom = PointerOwnershipModel(raw_pom_data)
        pom.set_file_path(pom_file_path)

        return pom

    def __str__(self) -> str:
        function_names = self.get_function_names()
        output = f"PointerOwnershipModel({self.file_path}, functions={function_names})"

        return output

    def __repr__(self) -> str:
        return self.__str__()


def read_clang_ast_file(clang_ast_file_path: str, verbose=False) -> OrderedDict:
    """Read a clang AST file (JSON or gzipped JSON)."""
    if verbose:
        log(f"read_clang_ast_file({clang_ast_file_path}, verbose={verbose})")

    fp = Path(clang_ast_file_path)

    if not fp.exists() or not fp.is_file():
        warn(f"AST file '{clang_ast_file_path}' not found or not a file.")

        sys.exit(1)

    try:
        if fp.suffix == ".gz":
            with gzip.open(fp, 'rt', encoding='utf-8') as f:
                data = json.load(f, object_pairs_hook=OrderedDict)
        else:
            with fp.open('r', encoding='utf-8') as f:
                data = json.load(f, object_pairs_hook=OrderedDict)

    except (OSError, json.JSONDecodeError) as e:
        warn(f"Failed to read AST file '{clang_ast_file_path}': {e}")

        sys.exit(1)

    if not data:
        warn(f"AST file '{clang_ast_file_path}' is empty or malformed.")
        sys.exit(1)

    return data


def verify_ast_functions_in_pom(pom: PointerOwnershipModel, ast: ASTNode, verbose=False) -> bool:
    """
    Helper function to verify that all functions in AST are found within POM
    """
    if verbose:
        log(f"verify_ast_functions_in_pom({pom}, {ast}, {verbose})")
    tracker = False

    for ast_func in ASTNode.get_all_functions(ast):
        name = ast_func.get_name()

        if verbose:
            log(f"checking AST functions in POM: {name}")

        if ast_func.sys_func:
            # this is a system function and skip
            continue

        if ast_func.body is None:
            # assumed system function and skip
            continue

        if name is None:
            error(
                f"FunctionDecl with id {ast_func.get_id()} does not have a name.")

            continue

        if name.startswith("__"):
            # assumed system function and skip
            continue

        for pom_name in pom.get_function_names():
            if name == pom_name:
                tracker = True
                break

        if not tracker:
            log(f"Could not find AST function '{name}' in POM.")

    return tracker


def verify_function_existence(name: str, ast: ASTNode, verbose=False) -> Tuple[bool, Optional[FunctionDecl]]:
    """
    Given a function name, verify it exists in a given AST node.

    Arguments:
        name (str): Function name, usually from POM
        ast (digest.ASTNode): a digested ASTNode

    Returns:
        (found, found_ast_func) (bool, digest.FunctionDecl | None): whether the function exists and if it does, the FunctionDecl node responsible for it, otherwise None
    """
    if verbose:
        log(f"verify_function_existence({name}, {ast}, verbose={verbose})")
    found = False
    found_ast_func = None

    for ast_func in ASTNode.get_all_functions(ast):
        ast_func_name = ast_func.get_name()

        if name == ast_func_name:
            found = True
            found_ast_func = ast_func

            if verbose:
                log(f"found function {name} in AST at node {found_ast_func.get_id()}")
            break

    if not found or not isinstance(found_ast_func, FunctionDecl) or found_ast_func.get_kind() != "FunctionDecl":
        warn(f"POM contains function '{name}' but AST does not.")

        # since the function is not found, skip verifing this
        return False, None

    return True, found_ast_func


def verify_function_arguments(pom_args: OrderedDict, func_ast: FunctionDecl, verbose=False) -> Tuple[bool, OrderedDict]:
    """
    Verify POM arguments against a given FunctionDecl node

    Arguments:
        pom_args (OrderedDict): Arguments from POM for a given function
        func_ast (digest.FunctionDecl): Associated AST function

    Returns:
        (passed, all_arg_verify) (bool, OrderedDict): if the POM correctly matches the AST and the associated raw data checks
    """
    if verbose:
        log(f"verify_function_arguments({pom_args}, {func_ast}, verbose={verbose})")

    all_arg_verify = OrderedDict()
    passed = True

    for ast_arg in func_ast.arguments:
        arg_verify = OrderedDict()

        arg_verify['exists'] = False

        # check if argument is a pointer
        if not is_ptr(ast_arg.get_type()):
            continue

        # check if AST argument is in POM
        arg_name = ast_arg.get_name()
        if arg_name not in pom_args:
            warn(
                f"AST contains argument '{arg_name}' in function '{func_ast.get_name()}' but POM does not.")

            passed = False
        else:
            arg_verify['exists'] = True

            if verbose:
                log(
                    f"found AST argument '{arg_name}' in function '{func_ast.get_name()}' within POM")

            # TODO: extend argument verification here

        all_arg_verify[arg_name] = arg_verify

    return passed, all_arg_verify


def verify_function_locals(pom_locals: OrderedDict, func_ast: FunctionDecl, verbose=False) -> Tuple[bool, OrderedDict]:
    """
    Verify POM local variable data against a AST FunctionDecl

    Arguments:
        pom_locals (OrderedDict): POM local variable data for a given function
        func_ast (digest.FunctionDecl): Associated AST function node

    Returns:
        (passed, all_local_verify) (bool, OrderedDict): if the POM correctly matches the AST and the associated raw data checks
    """
    if verbose:
        log(f"verify_function_locals({pom_locals}, {func_ast}, verbose={verbose})")

    all_local_verify = OrderedDict()
    passed = True

    for ast_local in func_ast.locals:
        local_verify = OrderedDict()

        local_verify['exists'] = False

        # check if local is a pointer
        if not is_ptr(ast_local.get_type()):
            continue

        # check if AST local is in POM
        local_name = ast_local.get_name()
        if local_name not in pom_locals:
            warn(
                f"AST contains local pointer variable '{local_name}' in function '{func_ast.get_name()}' but POM does not.")

            passed = False
        else:
            local_verify['exists'] = True

            if verbose:
                log(
                    f"found AST local pointer variable '{local_name}' in function '{func_ast.get_name()}' within POM")

            # TODO: extend local variable verification here

        all_local_verify[local_name] = local_verify

    return passed, all_local_verify


def verify_function_return(pom_return: OrderedDict | None, func_ast: FunctionDecl, verbose=False) -> bool:
    """
    Verify POM return data against an AST FunctionDecl

    Arguments:
        pom_return (OrderedDict): POM return data for a given function
        func_ast (digest.FunctionDecl): AST node for associated function

    Returns:
        passed (bool): True if the POM matches the AST, False otherwise
    """
    if verbose:
        log(f"verify_function_return({pom_return}, {func_ast}, {verbose})")

    ast_return_type: str | None = func_ast.return_type

    if ast_return_type is None:
        if pom_return is None:
            return True
        else:
            if pom_return == {}:
                # AST: void
                # POM: contains nothing
                return True
            else:
                warn(
                    f"AST return type is void for function '{func_ast.get_name()}' while POM contains return information")

                return False
    else:
        is_ast_rt_ptr = '*' in ast_return_type
        if pom_return is None:
            if not is_ast_rt_ptr:
                return True
            else:
                warn(
                    f"AST return type is '{ast_return_type}' for function '{func_ast.get_name()}' while POM is missing return information")
                return False
        else:
            if is_ast_rt_ptr and pom_return != {}:
                return True

            if not is_ast_rt_ptr and pom_return == {}:
                return True

            # TODO: extend verification of return

    return False


def verify_pom(pom: PointerOwnershipModel, ast: ASTNode, verbose=False) -> Tuple[bool, OrderedDict]:
    """
    Verify a Pointer Ownership Model against an AST.
    """
    if verbose:
        log(f"verify_pom({pom}, {ast}, verbose={verbose})")

    verify_data = OrderedDict()

    # verify that all functions in AST are found in POM
    verify_data['complete-ast-model'] = verify_ast_functions_in_pom(
        pom, ast, verbose)

    if verbose:
        log("finished verifying functions in AST are within POM")

    # verify all functions found in POM against AST
    for name in pom.get_functions():
        func_verify = OrderedDict()

        func_verify['exists'] = False
        func_verify['correct-arguments'] = False
        func_verify['correct-locals'] = False
        func_verify['correct-return'] = False

        func_verify['arguments'] = OrderedDict()
        func_verify['locals'] = OrderedDict()

        pom_data: OrderedDict = pom.get_function(name)

        # verify function existence
        func_verify['exists'], found_ast_func = verify_function_existence(
            name, ast, verbose)

        if found_ast_func is None:
            continue

        # verify arguments
        pom_args: OrderedDict = pom_data.get('args', {})
        func_verify['correct-arguments'], func_verify['arguments'] = verify_function_arguments(
            pom_args, found_ast_func, verbose)

        # verify locals
        pom_locals = pom_data.get('locals', {})
        func_verify['correct-locals'], func_verify['locals'] = verify_function_locals(
            pom_locals, found_ast_func, verbose)

        # verify return
        pom_return = pom_data.get('return', None)
        func_verify['correct-return'] = verify_function_return(
            pom_return, found_ast_func, verbose)

        # store func_verify in verify_data
        verify_data[name] = func_verify

    all_true = all(value for value in verify_data.values())
    return all_true, verify_data


def main():
    args = parse_args()
    verbose = bool(args.verbose)
    output = args.output

    try:
        pom = PointerOwnershipModel.read_from_file(
            args.pom_file, args.validate, verbose)

        if verbose:
            log(f"POM: {pom}")

    except Exception as e:
        logging.warning(
            f"Failed to read or validate structure of POM file: {e}")
        sys.exit(1)

    overall_success = True
    digested = None

    try:
        ast_data = read_clang_ast_file(args.clang_ast_file, verbose)
        digested = digest(ast_data)
        if not digested:
            warn(f"AST data from {args.clang_ast_file} was unable to be digested.")
            sys.exit(1)

        if args.verbose:
            log(f"Digested AST: {digested}")

        overall_success, pom_results = verify_pom(pom, digested, verbose)

        if output:
            with open(output, 'w') as f:
                json.dump(pom_results, f, ensure_ascii=True, indent=4)
        else:
            print(json.dumps(pom_results, ensure_ascii=True, indent=4))

        sys.exit(0 if overall_success else 1)
    except Exception as e:
        warn(
            f"Failed to read and digest AST file {args.clang_ast_file}: {e}")

        sys.exit(1)



if __name__ == "__main__":
    main()
