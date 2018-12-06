#!/bin/bash
set -ex

PYTHON_BIN=$(readlink -f $(which python))
PYTHON_ROOT=${PYTHON_BIN%/bin/python*}
PYTHON_VER=${PYTHON_BIN#$PYTHON_ROOT/bin/python}

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:${PYTHON_ROOT}/lib/pkgconfig
export CPATH=$CPATH:${PYTHON_ROOT}/include/python${PYTHON_VER}m

