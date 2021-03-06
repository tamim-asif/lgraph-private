#!/bin/bash
# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

# Defaults for configuration variables used by script
: ${LGRAPH_SRC:=${HOME}/lgraph}
: ${LGRAPH_BUILD_MODE:=fastbuild}

if [ ! -e ${LGRAPH_SRC}/WORKSPACE ]; then
  echo "ERROR: '${LGRAPH_SRC}' does not contain LGRAPH source code"
  exit -1
fi

echo "build.sh: Building....."
echo "build.sh: Getting sub directories..."
cd ${LGRAPH_SRC}
git submodule update --init --recursive
bazel build -c ${LGRAPH_BUILD_MODE} //...

