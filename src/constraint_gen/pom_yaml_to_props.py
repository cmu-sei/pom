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
    
    # Handle responsibility property and default mutability
    resp = None
    is_mut = None
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
            resp = "!responsible"
        output_lines.append(f"{resp.lower()}({path})")
    
    # Handle mutability property
    if 'mutable' in element:
        is_mut = element['mutable']
    if is_mut is None:
        if not path.endswith("::return"):
            sys.stderr.write(f"Warning: no mutability for {path}.\n")
    else:
        output_lines.append(("" if is_mut else "!") + f"mut({path})")
    
    # Handle lifetime property
    if 'lifetime' in element and ("::locals::" not in path):
        path_parts = path.split("::")
        func_name = path_parts[0].replace("*","")
        for src in element['lifetime']:
            output_lines.append(f"borrows_from({path}, {src})")

    # Handle argument index
    if 'arg_idx' in element:
        path_parts = path.split("::")
        if len(path_parts) != 3 or path_parts[1] != "args":
            sys.stderr.write(f"Warning: Unexpected 'arg_idx' attribute in {path}!\n")
        else:
            func_name = path_parts[0]
            arg_name = path_parts[2]
            glo.func_arg_index_names.append((func_name, element['arg_idx'], arg_name))
    
    # Handle start and end states at this level
    for timing in ['start', 'end']:
        if (timing not in element):
            continue
        if ("::locals::" in path):
            continue
        states = element[timing]
        if path.endswith("::return"):
            if timing == "start":
                if "end" in element:
                    sys.stderr.write("Error: return has both start and end states!")
                else:
                    timing = "end"
        if not isinstance(states, list):
            sys.stderr.write("Warning: %r is not a list!\n" % states)
            continue
        if (resp == "responsible"):
            states = list([x.lower() for x in states])
            for (ix, state) in enumerate(states):
                if state == "nul":
                    state = "null"
                    states[ix] = state
                output_lines.append(f"{state}({path}, {timing})")
            for poss_state in ["good", "null", "zombie"]:
                if poss_state not in states:
                    output_lines.append(f"!{poss_state}({path}, {timing})")
        else:
            if "NUL" not in str(states).upper():
                output_lines.append(f"!null({path}, {timing})")
            
                        
    
    # Handle referent (what this pointer points to)
    if 'referent' in element:
        referent = element['referent']
        referent_path = f"*{path}"
        process_element(referent, referent_path, output_lines)

def process_function(func_name, func_data, output_lines, opts):
    """Process a function and its arguments and return value."""
    
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
