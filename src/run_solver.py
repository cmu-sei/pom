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


import re
import argparse
import subprocess
import os
import logging
import sys
import yaml
import yamale
import json
import time
import shlex
from enum import Enum, auto
from dataclasses import dataclass
from typing import Optional, Mapping, Any
from io import StringIO

from constraint_gen.split_constraints import split_constraints
import constraint_gen.pom_yaml_to_props
import constraint_gen.conv_dimacs

DEFINED_FUNCTION_PATTERN = re.compile(r'<function name="(\w+)">')
CALLED_FUNCTION_PATTERN = re.compile(r'<instruction opcode="call" callee="(\w+)"')
PYTHON_INTERPRETER = "python3"
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
LLVM_OPTS = os.getenv("LLVM_OPTS", "")
POM_SCHEMA_FILE = os.environ.get("POM_SCHEMA_FILE", "/host/src/pom.yamale.yml")
POM_SCHEMA = yamale.make_schema(POM_SCHEMA_FILE)

IGNORE_CALLED_FUNCTIONS = {"__pom_var_store", "__assert_fail"}


class CommandFailed(Exception):
    def __init__(self, result: subprocess.CompletedProcess):
        self.result = result


class Satisfiability(Enum):
    UNDETERMINED = auto()
    SATISFIABLE = auto()
    UNSATISFIABLE = auto()


@dataclass
class SolverResult:
    satisfiability: Satisfiability
    proof: str = ""

use_new_python_process = False

def call_and_capture_stdout(func):
    ret = None
    old_stdout = sys.stdout
    sys.stdout = captured_output = StringIO()
    try:
        func()
        ret = captured_output.getvalue()
    finally:
        sys.stdout = old_stdout
    return ret

class TimingHandler(logging.Handler):
    def __init__(self):
        super().__init__()
        self.prev_msg = None
        self.prev_time = time.time()
        self.timings = {}

    def load(self, filename):
        try:
            with open(filename, "r") as infile:
                self.timings = json.load(infile)
        except:
            pass

    def save(self, filename):
        with open(filename, "w") as outfile:
            json.dump(self.timings, outfile)
            outfile.write("\n")
        
    def emit(self, record):
        msg = record.msg
        msg_split = msg.split(" ")
        num_words = 3
        if len(msg_split) >= 4:
            if msg_split[2] == PYTHON_INTERPRETER:
                num_words = 4
                msg_split[3] = os.path.basename(msg_split[3])
        msg = " ".join(msg_split[:num_words]) # Get first few words
        cur_time = time.time()
        if self.prev_msg:
            self.timings.setdefault(self.prev_msg, []).append(cur_time - self.prev_time)
        if msg.split(" ")[0].endswith("ing"):
            self.prev_msg = msg
        else:
            self.prev_msg = None
        self.prev_time = cur_time

# Configure logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

# Create stderr handler (for normal output)
stderr_handler = logging.StreamHandler(sys.stderr)
stderr_handler.setLevel(logging.WARNING)
stderr_handler.setFormatter(logging.Formatter(
    "[%(asctime)s %(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S"
))
logger.addHandler(stderr_handler)

# Add timing handler
timing_handler = None
def add_timing_handler():
    global timing_handler
    timing_handler = TimingHandler()
    timing_handler.setLevel(logging.DEBUG)
    logger.addHandler(timing_handler)


def run_command(
    command_string: str,
    env: Mapping[str, str] = os.environ,
    success_codes: list[int] = [0],
    directory: str = os.getcwd(),
) -> subprocess.CompletedProcess:
    """
    Run the given command in a subprocess, capturing the output and testing if
    the command was successful.  Raises ``CommandFailed`` if the command does
    not return one of the indicated return

    :param command_string: Command to run
    :param env: Environment variables to set in the subprocess's environment
    :param success_codes: List of return codes that indicate the command ran successfully
    :param directory: Directory in which the command should be run
    :returns: ``subprocess`` ``CompletedProcess`` result for the command that was run
    """
    logger.debug(f"Running command: {command_string}")
    result = subprocess.run(
        command_string,
        env=env,
        shell=True,
        capture_output=True,
        text=True,
        cwd=directory,
    )
    if result.returncode not in success_codes:
        logger.error(
            f'Command "{command_string}" failed (return code {result.returncode})'
        )
        logger.error(result.stdout)
        logger.error(result.stderr)
        raise CommandFailed(result)
    return result


def validate_pom_file(pom_file_path: str):
    """
    Validate that the POM file passed in complies with the POM schema.  If the
    validation fails, print an error and exit.

    :param pom_file_path: Filename of the POM file to validate
    """
    pom_data = yamale.make_data(pom_file_path)
    if (
        not pom_data
        or not pom_data[0]
        or not isinstance(pom_data[0], tuple)
        or not pom_data[0][0]
    ):
        logger.error(f"POM file '{pom_file_path}' is empty or malformed.")
        sys.exit(1)
    try:
        yamale.validate(POM_SCHEMA, pom_data)
        logger.info(f"POM file '{pom_file_path}' validated successfully.")
    except yamale.YamaleError as e:
        logger.error(f"POM file '{pom_file_path}' failed validation.")
        for result in e.results:
            logger.warning(f"Validation error: {result}")
        sys.exit(1)


def rebuild_constraint_builder():
    """
    Re-build the constraint generator object files, if necessary
    """
    logger.debug("Rebuilding constraint builder")
    run_command("make", directory=f"{SCRIPT_DIR}//constraint_gen")


def get_pom_functions(pom_list: Optional[list[str]]) -> list[str]:
    """
    Parse the given POM YAML file and return the list of functions it includes

    :param pom_list: List of names of POM files to parse, or ``None`` if none were specified
    :returns: A list of functions specified in the POM files
    """
    functions = []
    if pom_list:
        for pom_filename in pom_list:
            with open(pom_filename) as pom_file:
                pom_values = yaml.safe_load(pom_file)
                functions += pom_values["Functions"].keys()

    return functions


def merge_ll_files(ll_files: list[str], clang_ver: str, output_dir: str) -> str:
    """
    Combine all of the given .ll files together using ``llvm-link``

    :param ll_files: List of the filenames of the .ll files we want to combine
    :param clang_ver: Version of clang whose ``llvm-link`` we should use
    :param output_dir: Name of the directory where the merged file should be written
    :returns: If only one file was passed, the name of that file.  Otherwise,
        the name of the file containing the merged contents of all the .ll files.
    """
    if len(ll_files) == 1:
        return ll_files[0]
    else:
        command = f"llvm-link-{clang_ver}"
        merged_ll_file = f"{output_dir}/merged.ll"
        run_command(f"{command} -o {merged_ll_file} {' '.join(ll_files)}")
        return merged_ll_file


def generate_constraints_file(
        clang_ver: str, ll_file: str, output_dir: str, pom_list: Optional[list[str]], pom_library_list: Optional[list[str]]
):
    """
    Generate a ``constraints.txt`` file for the given .ll file (and a
    corresponding POM file, if given)

    :param clang_ver: Version of clang to use to generate the constraints file
    :param ll_file: .ll file to generate the constraints from
    :param output_dir: Directory to write the constraints file to
    :param pom_list: List of names of POM files to pull additional constraints from,
        or ``None`` if none were specified
    """
    logger.info(f"Generating constraints file for {ll_file}")

    # Generate POM constraints, stash in pom.constraints.txt, and create argnames_File
    pom_constraints = f"{output_dir}/pom.constraints.txt"
    if os.path.exists(pom_constraints):
        os.remove(pom_constraints)
    if pom_list is None:
        pom_list = []
    if pom_library_list is None:
        pom_library_list = []
    argnames_file = "/dev/null"
    if len(pom_list + pom_library_list) > 0:
        pom_files = " ".join(pom_list + pom_library_list)
        argnames_file = f"{output_dir}/famous_argnames.txt"
        args_for_yaml_to_props = f"{pom_files} --arg-name-file {argnames_file}"
        props = None
        if use_new_python_process:
            result = run_command(
                f"{PYTHON_INTERPRETER} {SCRIPT_DIR}/constraint_gen/pom_yaml_to_props.py {args_for_yaml_to_props}")
            props = result.stdout
        else:
            logger.debug(f"Calling pom_yaml_to_props.main")
            props = call_and_capture_stdout(lambda:
                constraint_gen.pom_yaml_to_props.main(shlex.split(args_for_yaml_to_props)))
        with open(pom_constraints, "a") as constraint_file:
            constraint_file.write(props)


    if ll_file.endswith(".raw.ll"):
        ll_ssa_file = ll_file.removesuffix(".raw.ll") + ".ssa.ll"
    elif ll_file.endswith(".ll"):
        ll_ssa_file = ll_file.removesuffix(".ll") + ".ssa.ll"
    else:
        logger.error(f"Unexpected .ll filename {ll_file}")
        sys.exit(1)

    # Use argnames_file for generating LLVM constraints
    if clang_ver and int(clang_ver) >= 17:
        run_command(
            f"opt-{clang_ver}  -load-pass-plugin {SCRIPT_DIR}/constraint_gen/VarStorePass.so  -passes=globalopt,sccp,dce,var-store,mem2reg {ll_file}  -S  -o {ll_ssa_file}"
        )
        run_command(
            f"opt-{clang_ver}  -load-pass-plugin {SCRIPT_DIR}/constraint_gen/ConstraintGenPass.so  -passes=constraint-gen  -output {output_dir}/constraints.txt  {LLVM_OPTS}  -pom-props {pom_constraints}   -arg-name-file {argnames_file}  -numir {output_dir}/numbered_ir.txt  {ll_ssa_file}  -S  -o /dev/null"
        )
    else:
        run_command(
            f"opt-15  -enable-new-pm=0  -load {SCRIPT_DIR}/constraint_gen/VarStorePass.so -globalopt -sccp -dce  -var-store  -mem2reg {ll_file}  -S  -o {ll_ssa_file}"
        )
        run_command(
            f"opt-15  -enable-new-pm=0  -load {SCRIPT_DIR}/constraint_gen/ConstraintGenPass.so  -constraint-gen  -output {output_dir}/constraints.txt  {LLVM_OPTS}    -pom-props {pom_constraints}  -arg-name-file {argnames_file}  -numir {output_dir}/numbered_ir.txt  {ll_ssa_file}  -S  -o /dev/null"
        )

    # Combine both constraints file into one.
    if pom_list or pom_library_list:
        run_command(
            f"cat  {pom_constraints}  >> {output_dir}/constraints.txt"
        )
        os.remove(pom_constraints)


def get_function_names(output_dir: str) -> tuple[list[str], list[str]]:
    """
    Parse the constraints file to extract the names of functions in the source

    :param output_dir: Directory where we should look for the ``constraints.txt`` file
    :returns: A tuple, with the first element containing the list of functions
        defined in the source, and the second containing the list of functions
        called in the source
    """
    defined_function_names = []
    called_function_names = []
    with open(f"{output_dir}/constraints.txt", "r") as constraints_file:
        for line in constraints_file:
            match_data = DEFINED_FUNCTION_PATTERN.match(line)
            if match_data:
                defined_function_names.append(match_data[1])
            else:
                match_data = CALLED_FUNCTION_PATTERN.match(line)
                if match_data:
                    called_function_names.append(match_data[1])

    return defined_function_names, called_function_names


def constraints_to_dimacs(output_dir: str, function_name: str):
    """
    Convert a constraints file to create a DIMACS file for a given function

    :param output_dir: Directory where we should look for the constraints file
    :param function_name: Name of the function we want to generate the DIMACS file for
    """
    logger.info(f"Generating .dimacs file for function {function_name}")
    args_for_conv_dimacs = f"--func {function_name} {output_dir}/constraints_{function_name}.txt -v {output_dir}/var_map.json -o {output_dir}/constraints_{function_name}.dimacs"
    if use_new_python_process:
        run_command(
            f"{PYTHON_INTERPRETER} {SCRIPT_DIR}/constraint_gen/conv_dimacs.py {args_for_conv_dimacs}"
        )
    else:
        constraint_gen.conv_dimacs.main(shlex.split(args_for_conv_dimacs))
        


def run_solver(
    dimacs_filename: str, function_name: str, output_dir: str
) -> Satisfiability:
    """
    Run the solver to determine if the given function is satisfiable

    :param dimacs_filename: DIMACS file for the given function
    :param function_name: Name of the function to be evaluated
    :param output_dir: Directory where the solution should be written
    :returns: A ``Satisfiability`` enum indicating whether the function is satisfiable
    """
    solution_filename = f"{output_dir}/solution_{function_name}.txt"
    env = {"FUNC": function_name}
    logger.info(f"Running solver to evaluate {function_name}()")
    result = run_command(
        f"minisat {dimacs_filename} {solution_filename}", env, success_codes=[10, 20]
    )
    if result.stdout.find("UNSATISFIABLE") >= 0:
        logger.info(f"{function_name}() is UNSATISFIABLE")
        return Satisfiability.UNSATISFIABLE
    elif result.stdout.find("SATISFIABLE") >= 0:
        logger.info(f"{function_name}() is SATISFIABLE")
        return Satisfiability.SATISFIABLE
    else:
        logger.error("Unable to parse solver output")
        return Satisfiability.UNDETERMINED


def get_unsat_core(function_name: str, output_dir: str) -> str:
    """
    Calculate a proof of unsatisfiability and unsat core for the given function

    :param function_name: Function to calculate the proof for
    :param output_dir: Directory to look for the DIMACS files, and to write the proofs to
    :returns: String containing the unsat core
    """
    run_command(
        f"cryptominisat {output_dir}/constraints_{function_name}.dimacs {output_dir}/proof_{function_name}.drat --verb 0",
        success_codes=[20],
    )
    run_command(
        f"/opt/drat-trim/drat-trim {output_dir}/constraints_{function_name}.dimacs {output_dir}/proof_{function_name}.drat -c {output_dir}/core_{function_name}.unsat"
    )
    run_command(
        f"{PYTHON_INTERPRETER} {SCRIPT_DIR}/constraint_gen/conv_dimacs.py --func {function_name} --from-dimacs {output_dir}/core_{function_name}.unsat  -o {output_dir}/core_{function_name}.unsat.named -v {output_dir}/var_map.json --clause-info-file {output_dir}/constraints.txt"
    )
    with open(f"{output_dir}/core_{function_name}.unsat.named") as unsat_core_file:
        return unsat_core_file.read()


def print_results_standard(
    function_values: dict[str, SolverResult],
    defined_but_not_in_pom: set[str],
    called_but_not_in_pom: set[str],
    in_pom_but_not_source: set[str]
):
    """
    Print out the results of the evaulation: A list of whether each function
    was satisfiable, and proofs for any that were unsatisfiable

    :param function_values: Dictionary mapping from a function name to the
        result of the evaluation for that function
    """
    print("Results")
    print("-------")

    if defined_but_not_in_pom:
        print()
        print(
            "The following functions are defined in the source, but are not the POM file(s):"
        )
        for f in defined_but_not_in_pom:
            print(f)

    if called_but_not_in_pom:
        print()
        print(
            "The following functions are called from the source, but are not in the POM file(s):"
        )
        for f in called_but_not_in_pom:
            print(f)

    if in_pom_but_not_source:
        print()
        print(
            "The following functions are in the POM file, but are neither called nor defined in the source:"
        )
        for f in in_pom_but_not_source:
            print(f)

    print()
    print("Function satisfiability:")
    function_names = sorted(function_values.keys())
    column_width = max([len(x) for x in function_names])
    show_proofs = False
    for function_name in sorted(function_values.keys()):
        print(
            f"{function_name.ljust(column_width)} {function_values[function_name].satisfiability.name}"
        )
        if (
            function_values[function_name].satisfiability
            == Satisfiability.UNSATISFIABLE
        ):
            show_proofs = True

    if show_proofs:
        print()
        print("Proofs:")
        print()
        for function_name in function_names:
            if (
                function_values[function_name].satisfiability
                == Satisfiability.UNSATISFIABLE
            ):
                print(f"{function_name}:")
                print(function_values[function_name].proof)


def print_results_json(
    function_values: dict[str, SolverResult],
    defined_but_not_in_pom: set[str],
    called_but_not_in_pom: set[str],
    in_pom_but_not_source: set[str]
):
    output: dict[str, Any] = {
        "function-satisfiability": {}
    }
    function_names = sorted(function_values.keys())
    show_proofs = False
    for function_name in sorted(function_values.keys()):
        satisfiability = function_values[function_name].satisfiability.name
        output["function-satisfiability"][function_name] = satisfiability
        if satisfiability == Satisfiability.UNSATISFIABLE:
            show_proofs = True

    if show_proofs:
        output["proofs"] = {}
        for function_name in function_names:
            if (
                function_values[function_name].satisfiability
                == Satisfiability.UNSATISFIABLE
            ):
                output["proofs"][function_name] = function_values[function_name].proof

    if defined_but_not_in_pom:
        output["defined-but-not-in-pom"] = sorted(list(defined_but_not_in_pom))
    if called_but_not_in_pom:
        output["called_but_not_in_pom"] = sorted(list(called_but_not_in_pom))
    if in_pom_but_not_source:
        output["in-pom-but-not-source"] = sorted(list(in_pom_but_not_source))

    print(json.dumps(output, indent=2))


def compare_function_lists(
    defined_functions: list[str],
    called_functions: list[str],
    pom_functions: list[str],
    library_functions: list[str],
) -> tuple[set[str], set[str], set[str]]:
    """
    Compare the list of functions in the source and the POM file, printing out
    any differences.

    :param defined_functions: List of functions defined in the source files
    :param called_functions: List of functions called in the source files
    :param pom_functions: List of functions specified in POM files
    :param library_functions: List of functions specified in _library_
        POM files, for which we shouldn't report functions not being called
    :returns: The 3 difference sets: defined-but-not-in-the-POM-files,
        called-but-not-in-the-POM-files, and in-the-POM-files-but-not-in-the-source
    """
    pom_set = set(pom_functions)
    defined_set = set(defined_functions)
    called_set = set(called_functions) - IGNORE_CALLED_FUNCTIONS
    library_set = set(library_functions)

    defined_but_not_in_pom = defined_set.difference(pom_set)
    called_but_not_in_pom = called_set.difference(pom_set | library_set)
    in_pom_but_not_source = pom_set.difference(defined_set | called_set)

    return defined_but_not_in_pom, called_but_not_in_pom, in_pom_but_not_source


def main(argv=None):
    # Process the command-line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("ll_files", nargs="+", help="LLVM IR file to process")
    parser.add_argument("--pom-file", "-p", action="append")
    parser.add_argument("--library-pom", action="append")
    parser.add_argument("--output-dir", "-o", default="out")
    parser.add_argument("--clang-ver", default="15")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--json", action="store_true", help="Output results in JSON format")
    parser.add_argument("--no-unsat-core", action="store_true", help="Don't compute unsat cores")
    parser.add_argument("--timings", type=str, help="Print timing data to specified file")
    parser.add_argument("--new-py-proc", action="store_true", help="Launch a new Python process for conv_dimacs and pom_yaml_to_props")
    args = parser.parse_args(argv)

    if args.timings:
        add_timing_handler()
        timing_handler.load(args.timings)
    
    global use_new_python_process
    use_new_python_process = args.new_py_proc or False

    if args.verbose:
        stderr_handler.setLevel(logging.DEBUG)

    # Validate the POM files, if any are given
    if args.pom_file:
        for pom_file in args.pom_file:
            validate_pom_file(pom_file)
    else:
        args.pom_file = []
    if args.library_pom:
        for pom_file in args.library_pom:
            validate_pom_file(pom_file)
    else:
        args.library_pom = []

    # Extract the list of functions from the POM files
    pom_functions = get_pom_functions(args.pom_file)
    library_functions = get_pom_functions(args.library_pom)

    # Make sure the output directory exists
    os.makedirs(args.output_dir, exist_ok=True)

    # Re-build the constraint generator, if necessary
    rebuild_constraint_builder()

    # Merge all the .ll files into a single one
    ll_file = merge_ll_files(args.ll_files, args.clang_ver, args.output_dir)

    # Generate the constraints file
    generate_constraints_file(
        args.clang_ver, ll_file, args.output_dir, args.pom_file, args.library_pom
    )

    # Split the constraints file by function
    logger.info("Splitting the constraints file by function")
    split_constraints(f"{args.output_dir}/constraints.txt", args.output_dir, quiet=True)

    # Parse the constraints file for function names
    defined_function_names, called_function_names = get_function_names(args.output_dir)

    # Loop through each function in the constraints file
    function_values = {}
    for function_name in defined_function_names:
        # Convert constraints file to a .dimacs file
        constraints_to_dimacs(args.output_dir, function_name)

        # Run the SAT solver
        result = run_solver(
            f"{args.output_dir}/constraints_{function_name}.dimacs",
            function_name,
            args.output_dir,
        )
        function_values[function_name] = SolverResult(satisfiability=result)

        # If the function was unsatisfiable, compute the proof for the unsatisfiability
        if result == Satisfiability.UNSATISFIABLE and not args.no_unsat_core:
            function_values[function_name].proof = get_unsat_core(
                function_name, args.output_dir
            )

    # Compare the functions in the constraints file vs the POM files
    defined_but_not_in_pom, called_but_not_in_pom, in_pom_but_not_source = (
        compare_function_lists(
            defined_function_names,
            called_function_names,
            pom_functions,
            library_functions,
        )
    )

    # Show the results
    if args.json:
        print_results_json(
            function_values,
            defined_but_not_in_pom,
            called_but_not_in_pom,
            in_pom_but_not_source
        )
    else:
        print_results_standard(
            function_values,
            defined_but_not_in_pom,
            called_but_not_in_pom,
            in_pom_but_not_source
        )

    if args.timings:
        timing_handler.save(args.timings)

if __name__ == "__main__":
    main()
