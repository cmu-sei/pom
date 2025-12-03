#!/usr/bin/env bash
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

# Usage: prefix a command with "./run_with_conv_dimacs_coproc.sh".

set -euo pipefail
coproc WORKER { python3 -u /host/src/constraint_gen/conv_dimacs.py --coproc; }  # WORKER[0]=stdout, WORKER[1]=stdin

# Duplicate to clear FD_CLOEXEC
exec {COPROC_IN}<>"/proc/self/fd/${WORKER[1]}"   # write end (to python stdin)
exec {COPROC_OUT}<>"/proc/self/fd/${WORKER[0]}"  # read end  (from python stdout)

# Pass the duplicated fds to children
export CONV_DIMACS_COPROC_STDIN=$COPROC_IN
export CONV_DIMACS_COPROC_STDOUT=$COPROC_OUT

# Run provided command
set +e
$@

echo "quit" >&"$CONV_DIMACS_COPROC_STDIN"

# close to let python exit, then wait
exec {COPROC_IN}>&-
exec {COPROC_OUT}<&-
wait "${WORKER_PID}" 2>/dev/null || true
