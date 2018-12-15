#!/bin/bash
set -e



function osx_graft_lib {
    libfile=$1
    deps=$(otool -L $libfile | awk '{print $1}')
    deppattern=$2
    new_libdir=.libs
    mkdir -p $new_libdir
    for dep in $deps; do
        if [ "$dep" == "$deppattern" ]; then
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
make -j4

if [ $(uname) == 'Darwin' ]; then
    osx_graft_lib cpp_household.so /usr/local/opt/python/Frameworks/Python.framework/Versions/3.6/Python 
    osx_graft_lib cpp_household.so /usr/local/opt/boost-python3/lib/libboost_python37.dylib
    osx_graft_lib cpp_household.so /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore
    osx_graft_lib cpp_household.so /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui
    osx_graft_lib cpp_household.so /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets
    osx_graft_lib cpp_household.so /usr/local/opt/qt/lib/QtOpenGL.framework/Versions/5/QtOpenGL
    osx_graft_lib cpp_household.so /usr/local/opt/assimp/lib/libassimp.4.dylib
fi
