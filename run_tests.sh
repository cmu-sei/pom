#!/bin/bash
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


docker run --rm -v ${PWD}:/host --workdir /host/src/test/ docker.cc.cert.org/pom/pointer-ownership-model rm test_verifier.xml
echo clean previous test outputs done

docker run --rm -v ${PWD}:/host --workdir /host/src/test/ docker.cc.cert.org/pom/pointer-ownership-model make clean asts
echo prep for verifier pytest done

docker run --rm -v ${PWD}:/host --workdir /host/src/test/ docker.cc.cert.org/pom/pointer-ownership-model pytest -v test_solver.py --junitxml=./test_solver.xml
echo verifier pytest done
