#!/bin/bash
set -e



function osx_graft_lib {
    libfile=$1
    deps=$(otool -L $libfile | awk '{print $1}')
    deppattern=$2
    new_libdir=.libs
    mkdir -p $new_libdir
    for dep in $deps; do
        if [[ "$dep" =~ $deppattern ]]; then
            new_libpath="$new_libdir/${dep##*/}"
            new_dep="@loader_path/$new_libpath"
            echo "$dep found, grafting to $new_libpath ($new_dep)"
            cp $dep $new_libdir/${dep##*/}
            install_name_tool -change $dep $new_dep $libfile
        fi
    done    
}

cd $(dirname "$0")

cd roboschool/cpp-household
make clean && make -j4
cd ..

if [ $(uname) == 'Darwin' ]; then
    osx_graft_lib cpp_household.so .+/Python
    osx_graft_lib cpp_household.so .+/libboost_python.*\.dylib
    osx_graft_lib cpp_household.so .+/QtCore
    osx_graft_lib cpp_household.so .+/QtGui
    osx_graft_lib cpp_household.so .+/QtWidgets
    osx_graft_lib cpp_household.so .+/QtOpenGL
    osx_graft_lib cpp_household.so .+/libassimp.*\.dylib
fi
