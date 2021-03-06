#!/bin/bash
# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

# Default for LGRAPH source code location
: LGRAPH_SRC=${LGRAPH_SRC:=${HOME}/lgraph}

if [ ! -e ${LGRAPH_SRC}/WORKSPACE ]; then
  echo "BUILD ERROR: '${LGRAPH_SRC}' does not contain LGRAPH source code"
  exit -1
fi

if ! ${LGRAPH_SRC}/scripts/build.sh
then
  echo "LGRAPH build error"
  exit -1
fi

${LGRAPH_SRC}/scripts/run-test.sh

