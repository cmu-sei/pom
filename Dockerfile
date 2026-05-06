# This file builds enough tools into a Linux distro to be useful for
# general software building.
# It can be used by the general public or for internal SEI usage with tailored commands, per comments

# docker build -f Dockerfile -t docker.cc.cert.org/pom/pointer-ownership-model:latest  .
# docker run -it --rm -v ${PWD}:/host -w /host docker.cc.cert.org/pom/pointer-ownership-model:latest bash

# To correct the time zone, run:
# dpkg-reconfigure tzdata

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

FROM ubuntu:noble
# For internal SEI use, substitute uncommented line below for line above
# FROM artifacts.cmu.edu/docker-virtual/ubuntu:noble

# Apt packages
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
         gcc mingw-w64 make autoconf zip unzip wget git gnupg ca-certificates tzdata \
    && apt-get --purge -y autoremove \
    && apt-get clean

# Clang (v15)
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
        clang-15 clang-tools-15 libclang-15-dev libclang1-15 clang-format-15 python3-clang-15 clangd-15 \
        llvm-15-dev cmake \
    && apt-get --purge -y autoremove \
    && apt-get clean \
    && ln -s /usr/bin/clang-15 /usr/bin/clang \
    && ln -s /usr/bin/clang++-15 /usr/bin/clang++

# Other packages
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
         bear sqlite3 po4a python3-pytest python3-pip iputils-ping iputils-tracepath patchutils \
         ninja-build lld zlib1g-dev libtinfo-dev libxml2-dev less \
         cryptominisat minisat \
    && apt-get --purge -y autoremove \
    && apt-get clean

#####################################################################
#
# add certificates required before cloning
#
#####################################################################

# if you receive any git-related errors due to certs then add your
# org's certs here
RUN /usr/sbin/update-ca-certificates



#####################################################################
#
# install Python packages
#
#####################################################################

RUN pip install  --no-cache-dir --break-system-packages  yamale aiofiles openai


#####################################################################
#
# install custom POM scripts
#
#####################################################################

RUN mkdir -p /opt/pom
COPY src/pom-lint /opt/pom/bash-scripts/pom-lint
COPY src/pom.yamale.yml /opt/pom/pom.yamale.yml
ENV PATH="/opt/pom/bash-scripts:$PATH"
ENV PYTHONPATH="/host/src"
ENV CLANG_VER=15
ENV LLVM_SYMBOLIZER_PATH="/usr/bin/llvm-symbolizer-$CLANG_VER"

# Extras for the SAT Solver
RUN mkdir /opt/drat-trim && \
    git clone https://github.com/marijnheule/drat-trim  /opt/drat-trim && \
    cd /opt/drat-trim && \
    make


# The below label is metadata only, so users external to the SEI won't get an error from it
LABEL org.opencontainers.image.source=https://bitbucket.cc.cert.org/bitbucket/projects/POM/repos/pointer-ownership-model/browse

CMD ["/bin/bash"]
