#!/bin/bash
# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

if [ "$#" -lt 1 ]; then
  echo "Usage: <lgraph_src_dir> [build_mode] [docker_image]"
  exit -1
fi

LGRAPH_SRC=$1
LGRAPH_BUILD_MODE=${2:fastbuild} # opt dbg fastbuild
DOCKER_IMAGE=${3:-mascucsc/archlinux-masc:latest}

DOCKER_LGRAPH_SRC='/root/lgraph'

if [ ! -e ${LGRAPH_SRC}/WORKSPACE ]; then
  echo "BUILD ERROR: '${LGRAPH_SRC}' does not contain LGRAPH source code"
  exit -1
fi

# possibly add back -t command later
docker run  \
  -v $LGRAPH_SRC:$DOCKER_LGRAPH_SRC \
  -e LGRAPH_SRC=${DOCKER_LGRAPH_SRC} \
  -e LGRAPH_BUILD_MODE=${LGRAPH_BUILD_MODE} \
  ${DOCKER_IMAGE} ${DOCKER_LGRAPH_SRC}/scripts/build-and-run.sh

