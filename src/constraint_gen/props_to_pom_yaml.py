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

import re
import sys, os
import argparse
import yaml
from collections import OrderedDict, defaultdict
import json
import pdb
import traceback
stop = pdb.set_trace

glo_lib_funcs = set()

def represent_list(dumper, data):
    # Use flow style (inline) for short lists with simple values
    if len(data) <= 3 and all(isinstance(item, (str, int, float, bool, type(None))) for item in data):
        return dumper.represent_sequence('tag:yaml.org,2002:seq', data, flow_style=True)
    return dumper.represent_sequence('tag:yaml.org,2002:seq', data, flow_style=False)

yaml.add_representer(list, represent_list)

def parse_predicate_line(line):
    """Parse a single predicate line and return (predicate, path, timing, negated)."""
    line = line.strip()
    if not line or "::" not in line:
        return None
    
    # Match patterns like: responsible(path), !mut(path), good(path, start)
    match = re.match(r'^(!?)(\w+)\(([^,)]+)(?:,\s*([a-zA-Z0-9_*.\[\]]+))?\)( = ([^#]*))?(#.*)?$', line)
    if not match:
        return None
    
    negated = match.group(1) == '!'
    predicate = match.group(2)
    path = match.group(3)
    timing = match.group(4)
    rhs = match.group(6)
    if rhs:
        rhs = rhs.strip()
        if negated:
            sys.stderr.write("Error: cannot have both a negation ('!') and a RHS.\n")
            sys.stderr.write(f"Offending line: {line}\n")
            return None
        if rhs.lower() == "true":
            rhs = None
        elif rhs.lower() == "false":
            negated = True
            rhs = None
        elif rhs.lower() in ["unassigned", "unknown", "indeterminate"]:
            return None
    
    return (predicate, path, timing, negated, rhs)

def parse_path(path):
    """Parse a path like 'foo::args::bar' or '*foo::args::bar' into components."""
    # Count leading asterisks for referent depth
    referent_depth = 0
    while path.startswith('*'):
        referent_depth += 1
        path = path[1:]
    
    parts = path.split('::')
    if len(parts) < 1:
        return None
    
    function = parts[0]
    section = parts[1]
    variable = parts[2] if len(parts) > 2 else None

    if variable and (":orig:" in variable):
        return None
    
    return {
        'function': function,
        'section': section,
        'variable': variable,
        'referent_depth': referent_depth
    }

def get_or_create_element(structure, func, section, var, depth):
    """Get or create an element at the specified path."""
    if func not in structure:
        structure[func] = {}
    if section not in structure[func]:
        structure[func][section] = {}
    
    if var:
        if var not in structure[func][section]:
            structure[func][section][var] = {}
        
        current = structure[func][section][var]
        
        # Navigate down referent chain
        for i in range(depth):
            if 'referent' not in current:
                current['referent'] = {}
            current = current['referent']
        
        return current
    else:
        # For return values - return the section itself
        current = structure[func][section]
        
        # Navigate down referent chain
        for i in range(depth):
            if 'referent' not in current:
                current['referent'] = {}
            current = current['referent']
        return current

def build_structure_from_props(lines):
    """Build the YAML structure from props lines."""
    structure = {}
    
    for line in lines:
        parsed = parse_predicate_line(line)
        if not parsed:
            continue
        
        (predicate, path, timing, negated, rhs) = parsed
        path_info = parse_path(path)
        if not path_info:
            continue
        
        func = path_info['function']
        section = path_info['section']
        var = path_info['variable']
        depth = path_info['referent_depth']

        if func in glo_lib_funcs:
            continue
        if section == "locals" and var == "__retval":
            continue
        
        element = get_or_create_element(structure, func, section, var, depth)

        def print_error(msg):
            sys.stderr.write("Error: " + msg)
            sys.stderr.write(f"Offending line: {line}\n")
        
        # Apply the predicate
        if predicate == 'responsible':
            if rhs != None:
                print_error(f"Invalid RHS '{rhs}' for predicate '{predicate}'\n")
                continue
            element['resp'] = 'responsible' if not negated else 'irresponsible'
        elif predicate == 'mut':
            if rhs == None:
                rhs = not negated
            else:
                m = re.match("^mut[(]([A-Za-z0-9_]+)[)]$", rhs)
                if not m:
                    print_error(f"Invalid RHS '{rhs}'\n")
                else:
                    rhs_arg = m.group(1)
                    rhs_arg_elem = get_or_create_element(structure, func, section, rhs_arg, depth=0)
                    if "mutable" not in rhs_arg_elem:
                        rhs_arg_elem["mutable"] = "true_or_false"
                    rhs = "=" + rhs
            element['mutable'] = rhs
        elif predicate == "borrows_from":
            if rhs != None:
                print_error(f"Invalid RHS '{rhs}' for predicate '{predicate}'\n")
                continue
            if not negated:
                element.setdefault("lifetime", []).append(timing)
            else:
                element.setdefault("lifetime", [])
                if timing in element["lifetime"]:
                    element["lifetime"].remove(timing)
        elif predicate in ['good', 'null', 'zombie']:
            if rhs != None:
                print_error(f"Invalid RHS '{rhs}' for predicate '{predicate}'\n")
                continue
            if timing not in ["start", "end"]:
                sys.stderr.write(f"Invalid time point '{timing}'\n")
                sys.stderr.write(f"Offending line: {line}\n")
                continue
            if timing not in element:
                element[timing] = ["GOOD"] # Include "GOOD" by default, unless specifically marked as false.
            state = predicate.upper() if predicate != 'null' else 'NUL'
            if state not in element[timing] and not negated:
                element[timing].append(state)
            if state in element[timing] and negated:
                element[timing].remove(state)
    
    return structure

def clean_element(element):
    """Recursively clean an element by removing default mutability."""
    if not isinstance(element, dict):
        return

    # Remove default mutability
    resp = element.get('resp')
    mut = element.get('mutable')
    if resp == 'responsible' and mut == True:
        # Default for responsible is mutable=true
        if 'mutable' in element:
            del element['mutable']
    elif resp == 'irresponsible' and mut == False:
        # Default for irresponsible is mutable=false
        if 'mutable' in element:
            del element['mutable']
    
    # Translate GOOD <-> VALID and ZOMBIE <-> INVALID
    for time_pt in ['start', 'end']:
        if time_pt in element:
            if resp == 'irresponsible':
                translation = {"GOOD":"VALID", "ZOMBIE":"INVALID"}
                element[time_pt] = [translation.get(x, x) for x in element[time_pt]]
            
    # Clean referent recursively
    if 'referent' in element:
        clean_element(element['referent'])

def clean_structure(structure):
    """Clean up the structure by removing default mutability and empty sections."""
    for func_name, func_data in structure.items():
        if isinstance(func_data, dict):
            for section_name, section_data in func_data.items():
                if section_name in ['args', 'locals'] and isinstance(section_data, dict):
                    for var_name, var_data in section_data.items():
                        clean_element(var_data)
                elif section_name == 'return':
                    clean_element(section_data)
    
    # Remove empty variables and sections
    for func_name in list(structure.keys()):
        func_data = structure[func_name]
        for section_name in list(func_data.keys()):
            section_data = func_data[section_name]
            if section_name in ['args', 'locals'] and isinstance(section_data, dict):
                # Remove empty variables
                for var_name in list(section_data.keys()):
                    if not section_data[var_name]:
                        del section_data[var_name]
                # Remove empty sections
                if not section_data:
                    del func_data[section_name]
            elif section_name == 'return' and not section_data:
                # Keep empty return sections as {}
                pass
        # Remove empty functions
        if not func_data:
            del structure[func_name]
    
    return structure

def convert_props_to_yaml(props_content):
    """Convert props content to YAML format."""
    lines = props_content.strip().split('\n')
    
    # Remove the <yaml_file> wrapper if present and filter out lines without ::
    filtered_lines = []
    for line in lines:
        line = line.strip()
        if line and '::' in line:
            filtered_lines.append(line)
    
    structure = build_structure_from_props(filtered_lines)
    structure = clean_structure(structure)
    
    # Build the final YAML structure
    result = {}
    if structure:
        if 'struct' in structure:
            result["Structs"] = structure['struct']
            del structure['struct']
        result['Functions'] = structure
    
    return yaml.dump(result, default_flow_style=False, sort_keys=False)

def read_json_file(filename):
    with open(filename, 'rt') as f:
        try:
            data = json.load(f, object_pairs_hook=OrderedDict)
        except Exception as e:
            sys.stderr.write("Error reading JSON file: {}: {}".format(filename, e))
            sys.exit(1)
    return data

def main():
    parser = argparse.ArgumentParser(description='Convert props format back to POM YAML')
    parser.add_argument('input', nargs='?', help='Input props file (default: stdin)')
    parser.add_argument('-o', '--output', help='Output YAML file (default: stdout)')
    parser.add_argument('--lib', help='The ".pom.yml" file for library functions')
    parser.add_argument('--debug',  action="store_true", help="Don't catch exceptions")
    
    args = parser.parse_args()

    if args.lib:
        with open(args.lib, "r") as lib_file:
            lib_yaml = yaml.safe_load(lib_file)
            global glo_lib_funcs
            glo_lib_funcs = set(lib_yaml["Functions"].keys())
    
    # Read input
    if args.input:
        if args.input.endswith(".json"):
            soln = read_json_file(args.input)
            lines = []
            for (prop, truth_val) in soln.items():
                assert(truth_val is True or truth_val is False)
                cur_line = ("!" if truth_val==False else "") + prop
                lines.append(cur_line)
            props_content = "\n".join(lines) + "\n"
        else:
            try:
                with open(args.input, 'r') as f:
                    props_content = f.read()
            except IOError as e:
                print(f"Error reading input file: {e}", file=sys.stderr)
                sys.exit(1)
    else:
        props_content = sys.stdin.read()
    
    class DummyException(Exception):
        pass

    # Convert
    try:
        yaml_content = convert_props_to_yaml(props_content)
    except (DummyException if args.debug else Exception) as e:
        print(f"Error converting props to YAML: {e}", file=sys.stderr)
        traceback.print_exc()
        sys.exit(1)
    
    # Add a blank line before each function
    yaml_content = re.sub(r'(?m)(?=^  [^ ])', '\n', yaml_content)

    # Write output
    if args.output:
        try:
            with open(args.output, 'w') as f:
                f.write(yaml_content)
        except IOError as e:
            print(f"Error writing output file: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        sys.stdout.write(yaml_content)

if __name__ == '__main__':
    main()
