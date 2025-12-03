#!/usr/bin/env python3
"""
parse_run_clang.py

Parses a shell script that contains clang build commands and produces a JSON
mapping of:

  source_file_path  -> { "ast": <AST file>, "ll": <IR file> }

The script looks for lines that match the pattern:

  /opt/clang/build/bin/clang -c … <source> … -Xclang -ast-dump=json … | gzip > $ast_out_dir/<hash>.raw.ast.json.gz
  /opt/clang/build/bin/clang -c … <source> … -emit-llvm … -o $ast_out_dir/<hash>.raw.ll

It extracts the source file and the two output filenames, then writes a JSON
object to stdout (or to a file if you pass an argument).

Usage:
    python3 parse_run_clang.py /path/to/run_clang.sh > mapping.json
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

import json
import re
import sys
from pathlib import Path

# --------------------------------------------------------------------------- #
# 1. Regular expressions ----------------------------------------------------- #
# --------------------------------------------------------------------------- #

# Matches the clang command that produces the AST.
AST_RE = re.compile(
    r"""
    ^\s*\$CLANG                             # clang binary
    .*\s                                    # everything up to the source file
    (?P<src>[^\s]+\.c)                      # <source>.c
    \s+.*?                                  # other flags up to -Xclang
    -Xclang\s+-ast-dump=json                # AST dump flag
    .*?                                     # anything else
    \|[^>]+>\s*\$ast_out_dir/(?P<hash>[^\s]+)\.raw\.ast\.json\.gz
    """,
    re.VERBOSE,
)

# Matches the clang command that produces the LLVM IR.
LL_RE = re.compile(
    r"""
    ^\s*\$CLANG                             # clang binary
    .*\s
    (?P<src>[^\s]+\.c)                      # <source>.c
    \s+.*?                                  # other flags up to -emit-llvm
    -emit-llvm\s
    .*\s
    -o\s*\$ast_out_dir/(?P<hash>[^\s]+)\.raw\.ll
    """,
    re.VERBOSE,
)

# --------------------------------------------------------------------------- #
# 2. Helper functions -------------------------------------------------------- #
# --------------------------------------------------------------------------- #

def parse_line(line: str):
    """Try to match a line against the AST or LL regexes."""
    m_ast = AST_RE.search(line)
    if m_ast:
        return ("ast", m_ast.group("src"), m_ast.group("hash") + ".raw.ast.json.gz")

    m_ll = LL_RE.search(line)
    if m_ll:
        return ("ll",  m_ll.group("src"), m_ll.group("hash") + ".raw.ll")

    return None


def collect_mappings(lines):
    """Return a dict mapping source file -> {ast:..., ll:...}."""
    mappings = {}
    for line in lines:
        res = parse_line(line)
        if not res:
            continue
        kind, src, out_file = res
        # Initialise dict if needed
        mappings.setdefault(src, {})
        # Store the output filename (only the basename, as in your example)
        mappings[src][kind] = out_file
    return mappings


# --------------------------------------------------------------------------- #
# 3. Main entry point -------------------------------------------------------- #
# --------------------------------------------------------------------------- #

def main(argv):
    if len(argv) != 2:
        print(f"Usage: {argv[0]} <run_clang.sh>", file=sys.stderr)
        sys.exit(1)

    script_path = Path(argv[1]).resolve()
    if not script_path.is_file():
        print(f"Error: {script_path} does not exist or is not a file", file=sys.stderr)
        sys.exit(1)

    with script_path.open() as f:
        lines = f.readlines()

    mappings = collect_mappings(lines)

    # Pretty‑print JSON to stdout
    json.dump(mappings, sys.stdout, indent=2)
    print()  # newline


if __name__ == "__main__":
    main(sys.argv)
