#!/bin/bash

export ROBOSCHOOL_PATH=$(dirname "$0")
export CPP_HOUSEHOLD=$ROBOSCHOOL_PATH/roboschool/cpp-household

PYTHON_BIN=$(readlink -f $(which python))
PYTHON_ROOT=${PYTHON_BIN%/bin/python*}
PYTHON_VER=${PYTHON_BIN#$PYTHON_ROOT/bin/python}

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:${PYTHON_ROOT}/lib/pkgconfig
export CPATH=$CPATH:${PYTHON_ROOT}/include/python${PYTHON_VER}m
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CPP_HOUSEHOLD/bullet_local_install/lib

