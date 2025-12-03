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

import yaml
import os, sys
import argparse
import re
import pdb
stop = pdb.set_trace

class Glo:
    pass

glo = Glo()

def process_element(element, path, output_lines):
    """Process a POM element and generate output lines."""
    
    # Handle simple responsibility string
    if isinstance(element, str):
        process_element({"resp": element}, path, output_lines)
        return
    
    if not isinstance(element, dict):
        return

    if element == {}:
        return

    def print_error(msg):
        sys.stderr.write(msg)
        output_lines.append("# " + msg.rstrip())

    # Handle responsibility property and default mutability
    resp = None
    is_mut = None
    missing_preds = []
    mut_default_by_resp = {
        "producer": True,
        "diligent": False,
        "responsible": True,
        "irresponsible": False,
    }
    if 'resp' in element:
        resp = element['resp']
        is_mut = mut_default_by_resp.get(resp)
        if resp in ["irresponsible", "diligent", "producer"]:
            #if resp == "producer" and glo.use_default_states:
            #    sub_elem = element
            #    while 'referent' in sub_elem:
            #        sub_elem = sub_elem['referent']
            #        if "start" not in sub_elem:
            #            if sub_elem.get("resp") == "responsible":
            #                sub_elem["start"] = ["NULL", "ZOMBIE"]
            #            else:
            #                sub_elem["start"] = ["GOOD", "NULL", "ZOMBIE"]
            #        if "end" not in sub_elem:
            #            sub_elem["end"] = ["GOOD"]
            resp = "!responsible"
        elif resp != "responsible":
            print_error(f"Error: Bad value for 'resp': '{resp}' (path: {path})\n")
            missing_preds.append("responsible")
        output_lines.append(f"{resp.lower()}({path})")
    
    # Handle mutability property
    got_valid_mut = False
    if 'mutable' in element:
        is_mut = element['mutable']
    if is_mut is None:
        print_error(f"Warning: no mutability for {path}.\n")
    else:
        if is_mut in [True, False]:
            output_lines.append(("" if is_mut else "!") + f"mut({path})")
            got_valid_mut = True
        elif type(is_mut)==str and is_mut in ["varies", "true_or_false"]:
            glo.arg_mut[path.split(":")[-1]] = "varies"
            got_valid_mut = True
        elif type(is_mut)==str:
            m = re.match("^=?(mut|mutable)[(]([A-Za-z0-9_]+)[)]$", is_mut.strip())
            if not m:
                print_error(f"Invalid RHS '{is_mut}' specified for mutability of path {path}\n")
            else:
                rhs_arg = m.group(2)
                if glo.arg_mut.get(rhs_arg) != "varies":
                    print_error(f"Warning: mutability of {path} depends on mut({rhs_arg}), but mut({rhs_arg}) is not declared 'varies'/'true_or_false'.\n")
                output_lines.append(f"mut({path}) = mut({rhs_arg})")
                got_valid_mut = True
        else:
            print_error(f"Error for '{path}': bad value for 'mutable'.\n")

    if not got_valid_mut:
        missing_preds.append("mut")
    
    # Handle lifetime property
    if 'lifetime' in element and ("::locals::" not in path):
        path_parts = path.split("::")
        func_name = path_parts[0].replace("*","")
        for src in element['lifetime']:
            if src == "static":
                if is_mut and 'referent' in element:
                    print_error(f"Warning: static lifetime for mutable pointers-to-pointers is not supported ({path}).\n")
                continue
            output_lines.append(f"borrows_from({path}, {src})")

    # Handle argument index
    if 'arg_idx' in element:
        path_parts = path.split("::")
        if len(path_parts) != 3 or path_parts[1] != "args":
            print_error(f"Warning: Unexpected 'arg_idx' attribute in {path}!\n")
        else:
            func_name = path_parts[0]
            arg_name = path_parts[2]
            glo.func_arg_index_names.append((func_name, element['arg_idx'], arg_name))
    
    # Fixup for return value
    if path.endswith("::return"):
        if "start" in element:
            if "end" in element:
                print_error("Error: return has both start and end states!\n")
            else:
                element["end"] = element["start"]
            del element["start"]

    # Default to end=start if end is not specified
    if ("end" not in element) and ("start" in element):
        element["end"] = element["start"]
    
    # Handle start and end states at this level
    for timing in ['start', 'end']:
        if ("::locals::" in path or "struct::" in path):
            continue
        if timing=="end" and ("::args::" in path) and not (path.startswith("*") or path.endswith("[0]")):
            # The end state of an argument (but not the referent of an arg) is not needed.
            continue
        if path.endswith("::return") and timing == "start":
            continue
        if (timing not in element):
            if path in ["main::args::argv", "*main::args::argv"]:
                element[timing] = ["GOOD"]
            elif glo.use_default_states:
                states = ["GOOD"]
                element[timing] = states
            else:
                missing_preds.append(timing)
                continue
        states = element[timing]
        if not isinstance(states, list):
            print_error("Error: %r is not a list!\n" % states)
            missing_preds.append(timing)
            continue
        if resp != "responsible":
            translation = {"VALID":"GOOD", "INVALID":"ZOMBIE"}
            states = [translation.get(x.upper(), x) for x in states]
        states = list([x.lower() for x in states])
        for (ix, state) in enumerate(states):
            if state == "nul":
                state = "null"
                states[ix] = state
            if state not in ["good", "null", "zombie"]:
                print_error(f"Error: Unrecognized state '{state}' (path: {path})!\n")
            output_lines.append(f"{state}({path}, {timing})")
        for poss_state in ["good", "null", "zombie"]:
            if poss_state not in states:
                output_lines.append(f"!{poss_state}({path}, {timing})")
    if element.get("type") in ["array"]:
        output_lines.append(f"# Warning: {path}: arrays are not supported!")
    else:
        for pred in missing_preds:
            missing_line = f"{pred}({path}) = missing!"
            output_lines.append(missing_line.ljust(39) + f" # {glo.cur_file}")
    
    # Handle referent (what this pointer points to)
    if 'referent' in element:
        referent = element['referent']
        referent_path = f"*{path}"
        process_element(referent, referent_path, output_lines)

def process_function(func_name, func_data, output_lines, opts):
    """Process a function and its arguments and return value."""

    glo.arg_mut = {}
    
    # Process arguments
    if 'args' in func_data:
        args = func_data['args']
        if isinstance(args, dict):
            for arg_name, arg_data in args.items():
                arg_path = f"{func_name}::args::{arg_name}"
                process_element(arg_data, arg_path, output_lines)
    
    # Process locals
    if 'locals' in func_data and opts.get("include_locals"):
        local_vars = func_data['locals']
        if isinstance(local_vars, dict):
            for local_name, local_data in local_vars.items():
                local_path = f"{func_name}::locals::{local_name}"
                process_element(local_data, local_path, output_lines)
    
    # Process return value
    if 'return' in func_data:
        return_data = func_data['return']
        return_path = f"{func_name}::return"
        process_element(return_data, return_path, output_lines)

def process_struct(struct_name, struct_data, output_lines, opts):
    assert(isinstance(struct_data, dict))
    for field_name, field_data in struct_data.items():
        field_path = f"struct::{struct_name}::{field_name}"
        process_element(field_data, field_path, output_lines)

def convert_yaml_to_props(yaml_content, opts):
    """Convert YAML content to props format."""
    try:
        data = yaml.safe_load(yaml_content)
    except yaml.YAMLError as e:
        print(f"Error parsing YAML: {e}", file=sys.stderr)
        return None
    
    if not isinstance(data, dict) or 'Functions' not in data:
        print("Error: YAML must contain a 'Functions' key", file=sys.stderr)
        return None
    
    functions = data['Functions']
    if not isinstance(functions, dict):
        print("Error: 'Functions' must be a dictionary", file=sys.stderr)
        return None

    output_lines = []
    
    structs = data.get('Structs')
    if isinstance(structs, dict):
        for struct_name, struct_data in structs.items():
            process_struct(struct_name, struct_data, output_lines, opts)

    for func_name, func_data in functions.items():
        process_function(func_name, func_data, output_lines, opts)
    
    return output_lines

def remove_dups(L):
    seen = set()
    out = []
    for item in L:
        if item in seen:
            continue
        out.append(item)
        seen.add(item)
    return out

def main(argv=None):
    parser = argparse.ArgumentParser(description='Convert POM YAML to props format')
    parser.add_argument('input', nargs='*', help='Input YAML files (default: stdin if none provided)')
    parser.add_argument('-o', '--output', help='Output props file (default: stdout)')
    parser.add_argument('-a', '--arg-name-file', type=str, help='Output file mapping arg index to arg name')
    parser.add_argument('--use-default-states', type=str, help='Use default values for start and end if they are absent (also env var POM_USE_DEFAULT_STATES)')
    parser.add_argument('--locals', type=str, help='Process local vars too (default: env var POM_YAML_LOCALS)')
    
    args = parser.parse_args(argv)
    glo.func_arg_index_names = []

    # Delete output files if they exist (so that failures here are noisy rather than silently ignored)
    for out_file in [args.arg_name_file, args.output]:
        if out_file and os.path.exists(out_file):
            os.remove(out_file)
    
    str_to_bool_map = {
        "0": False, "false": False, "no": False,
        "1": True, "true": True, "yes": True
    }

    if args.use_default_states != None:
        glo.use_default_states = args.use_default_states
    else:
        glo.use_default_states = os.getenv("POM_USE_DEFAULT_STATES", default="True")
    glo.use_default_states = str_to_bool_map.get(glo.use_default_states.lower(), True)

    # Determine whether to process local variables
    include_locals = True
    if args.locals:
        include_locals = str_to_bool_map.get(args.locals.lower(), None)
        if include_locals is None:
            sys.stderr.write("Expecting '--locals [true|false]'\n")
            sys.exit(1)
    elif os.getenv("POM_YAML_LOCALS"):
        include_locals = str_to_bool_map.get(os.getenv("POM_YAML_LOCALS"), None)
        if include_locals is None:
            sys.stderr.write("Expecting 'true' or 'false' for value of env var POM_YAML_LOCALS.\n")
            sys.exit(1)

    # Read and process inputs
    all_props_content = []
    
    if not args.input:  # No input files provided, read from stdin
        yaml_content = sys.stdin.read()
        glo.cur_file = "<stdin>"
        props_content = convert_yaml_to_props(yaml_content, {"include_locals": include_locals})
        if props_content is None:
            sys.exit(1)
        all_props_content.extend(props_content)
    else:  # Process each input file
        for input_file in args.input:
            try:
                with open(input_file, 'r') as f:
                    yaml_content = f.read()
            except IOError as e:
                print(f"Error reading input file {input_file}: {e}", file=sys.stderr)
                sys.exit(1)
            
            glo.cur_file = input_file
            props_content = convert_yaml_to_props(yaml_content, {"include_locals": include_locals})
            if props_content is None:
                sys.exit(1)
            all_props_content.extend(props_content)
    
    # Combine all props content
    combined_props_content = (
        "<yaml_file>\n" +
        '\n'.join(remove_dups(all_props_content)) + "\n" +
        "</yaml_file>\n"
    )
    
    # Write output
    if args.output:
        try:
            with open(args.output, 'w') as f:
                f.write(combined_props_content)
        except IOError as e:
            print(f"Error writing output file: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        sys.stdout.write(combined_props_content)
    
    # Write (func_name, arg_idx, arg_name) info
    if args.arg_name_file:
        arg_name_content = "".join(
            f"{func_name} {arg_idx} {arg_name}\n"
            for (func_name, arg_idx, arg_name) in remove_dups(sorted(glo.func_arg_index_names)))
        try:
            with open(args.arg_name_file, 'w') as f:
                f.write(arg_name_content)
        except IOError as e:
            print(f"Error writing output file: {e}", file=sys.stderr)
            sys.exit(1)

if __name__ == '__main__':
    main()
