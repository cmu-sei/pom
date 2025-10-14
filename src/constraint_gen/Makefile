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

# Makefile for LLVM Passes

# LLVM version - can be overridden on command line
CLANG_VER ?= 15

CXX = clang++
LLVM_CONFIG = llvm-config-$(CLANG_VER)

# Get LLVM compilation flags
CXXFLAGS = `$(LLVM_CONFIG) --cxxflags` -std=c++17 -g -fPIC -fno-rtti -DCLANG_VER=$(CLANG_VER) $(OPT_FLAG)
LDFLAGS = `$(LLVM_CONFIG) --ldflags`
LIBS = `$(LLVM_CONFIG) --libs`

# Source files and targets
CONSTRAINT_SOURCES = ConstraintGenPass.cpp
VARSTORE_SOURCES = VarStorePass.cpp

CONSTRAINT_OBJECTS = $(CONSTRAINT_SOURCES:.cpp=.o)
VARSTORE_OBJECTS = $(VARSTORE_SOURCES:.cpp=.o)

CONSTRAINT_TARGET = ConstraintGenPass.so
VARSTORE_TARGET = VarStorePass.so

ALL_OBJECTS = $(CONSTRAINT_OBJECTS) $(VARSTORE_OBJECTS)
ALL_TARGETS = $(CONSTRAINT_TARGET) $(VARSTORE_TARGET)

.PHONY: all clean help constraint-gen var-store

all: $(ALL_TARGETS)

constraint-gen: $(CONSTRAINT_TARGET)

var-store: $(VARSTORE_TARGET)

$(CONSTRAINT_TARGET): $(CONSTRAINT_OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

$(VARSTORE_TARGET): $(VARSTORE_OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL_OBJECTS) $(ALL_TARGETS)

help:
	@echo "Usage:"
	@echo "  make                   # Build both passes with LLVM 15 (default)"
	@echo "  make constraint-gen    # Build only ConstraintGenPass"
	@echo "  make var-store         # Build only VarStorePass"
	@echo "  make CLANG_VER=15      # Build with LLVM 15"
	@echo "  make CLANG_VER=18      # Build with LLVM 18"
	@echo "  make clean             # Clean build files"
	@echo ""
	@echo "Current configuration:"
	@echo "  CLANG_VER = $(CLANG_VER)"
	@echo "  LLVM_CONFIG = $(LLVM_CONFIG)"

install: $(ALL_TARGETS)
	cp $(ALL_TARGETS) `$(LLVM_CONFIG) --libdir`/

.PHONY: test-15 test-18 test-varstore-15 test-varstore-18
test-15: $(CONSTRAINT_TARGET) $(VARSTORE_TARGET)
	clang-15 -g -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names  ../test/join_good_zombie.c -o join_good_zombie.raw.ll
	opt-15 -enable-new-pm=0 -load ./VarStorePass.so -var-store -mem2reg join_good_zombie.raw.ll -S -o join_good_zombie.ssa.ll
	opt-15 -enable-new-pm=0 -load ./ConstraintGenPass.so -constraint-gen -output out/constraints.txt --no-check-null join_good_zombie.ssa.ll -S -o /dev/null

test-18: $(CONSTRAINT_TARGET) $(VARSTORE_TARGET)
	clang-18 -g -O0 -Xclang -fexperimental-assignment-tracking=enabled -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names -Xclang -disable-llvm-passes ../test/join_good_zombie.c -o join_good_zombie.raw.ll
	opt-18 -load-pass-plugin ./VarStorePass.so -passes=var-store,mem2reg join_good_zombie.raw.ll -S -o join_good_zombie.ssa.ll
	opt-18 -load-pass-plugin ./ConstraintGenPass.so -passes=constraint-gen -output out/constraints.txt --no-check-null join_good_zombie.ssa.ll -S -o /dev/null
