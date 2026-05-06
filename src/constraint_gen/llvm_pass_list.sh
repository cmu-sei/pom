#
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
#


SCRIPT_DIR="$( dirname "${BASH_SOURCE[0]}" )"

CLANG_15_INITIAL_LOADS="-load $SCRIPT_DIR/VarStorePass.so -load $SCRIPT_DIR/FuncPtrPass.so -load $SCRIPT_DIR/FuncPtrExpander.so -load $SCRIPT_DIR/InternalizeGlobals.so -load $SCRIPT_DIR/FreezeUB.so"
CLANG_18_INITIAL_LOADS="-load-pass-plugin $SCRIPT_DIR/VarStorePass.so -load-pass-plugin $SCRIPT_DIR/FuncPtrPass.so -load-pass-plugin $SCRIPT_DIR/FuncPtrExpander.so $SCRIPT_DIR/InternalizeGlobals.so -load $SCRIPT_DIR/FreezeUB.so"


if [ "$HAVE_WHOLE_PROG" == "true" ]; then
    internalize_pass="-internalize-globals"
else
    internalize_pass=""
fi

if [ "$SKIP_FUNC_PTR" == "true" ]; then
    CLANG_18_FUNC_PTR_PASSES=""
    CLANG_15_FUNC_PTR_PASSES=""
else
    CLANG_18_FUNC_PTR_PASSES=",funcptr-analysis,expand-indirect-calls"
    CLANG_15_FUNC_PTR_PASSES="-funcptr-analysis -expand-indirect-calls"
fi

CLANG_18_INITIAL_PASSES="globalopt,sccp,dce,var-store,mem2reg$CLANG_18_FUNC_PTR_PASSES"
#CLANG_15_INITIAL_PASSES="$internalize_pass -globalopt -sccp -dce -var-store -mem2reg -funcptr-analysis -expand-indirect-calls"
CLANG_15_INITIAL_PASSES="$internalize_pass -var-store -mem2reg -freeze-ub -attributor-cgscc -ipsccp -globalopt -dce $CLANG_15_FUNC_PTR_PASSES"
