#
# Makefile for tests
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


SOURCES := $(wildcard *.c)
ASTS := $(SOURCES:.c=.raw.ast.json.gz)
LLS := $(SOURCES:.c=.raw.ll)


%.raw.ast.json.gz: %.c
	clang -c $< -Xclang -ast-dump=json -fsyntax-only | gzip > $@

%.raw.ll: %.c
	clang-15 -ggdb -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names $< -o $@

%.verifier.json: %.c %.raw.ast.json.gz %.raw.ll
	cd .. && python3 verify.py test/$(basename $<).pom.yml test/$(basename $<).raw.ast.json.gz --output test/$@ && cd test

%.verify: %.c %.raw.ast.json.gz
	python3 ../verify.py --verbose $(basename $<).pom.yml $(basename $<).raw.ast.json.gz

asts: $(ASTS) $(LLS)

clean:
	-rm *.raw.ast.json.gz *.raw.ll *.ssa.ll *.o

.PHONY: clean
.PRECIOUS: %.raw.ast.json.gz %.raw.ll %.output
