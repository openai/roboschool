export ROBOSCHOOL_PATH=$( cd "$(dirname "$BASH_SOURCE")" ; pwd -P )
export CPP_HOUSEHOLD=$ROBOSCHOOL_PATH/roboschool/cpp-household
export BULLET_PATH=$HOME/.forked_bullet

if [[ $(uname) == 'Linux' ]]; then
    PYTHON_BIN=$(readlink -f $(which python))
    PYTHON_ROOT=${PYTHON_BIN%/bin/python*}
    PYTHON_VER=${PYTHON_BIN#$PYTHON_ROOT/bin/python}
fi
if [[ $(uname) == 'Darwin' ]]; then
    brew install coreutils
    PYTHON_BIN=$(greadlink -f $(which python))
    PYTHON_ROOT=${PYTHON_BIN%/bin/python*}
    PYTHON_VER=${PYTHON_BIN#$PYTHON_ROOT/bin/python}
    export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/opt/qt/lib/pkgconfig:/Library/Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/pkgconfig/
fi


export PKG_CONFIG_PATH=${PYTHON_ROOT}/lib/pkgconfig:$BULLET_PATH/lib/pkgconfig/:$PKG_CONFIG_PATH
export CPATH=$CPATH:${PYTHON_ROOT}/include/python${PYTHON_VER}m:$HOME/.boost/include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BULLET_PATH/lib:$HOME/.boost/lib

