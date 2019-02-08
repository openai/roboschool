#!/bin/bash
set -e

function graft_libs {
    local libfile=$1
    local libdir=$(dirname $libfile)
    if [ $(uname) == 'Darwin' ]; then
        local relink="osx_relink"
        local origin="@loader_path"
        local deps=$(otool -L $libfile | awk 'FNR>2 {print $1}')
    fi
    if [ $(uname) == 'Linux' ]; then
        local relink="linux_relink"
        local library_lister="ldd"
        local origin="\$ORIGIN"
        local deps=$(ldd $libfile | awk '{print $3}')
    fi

    local patterns=${@:3}
    
    graft_dir=$2
    mkdir -p $graft_dir

    for dep in $deps; do
        # echo $dep
        for deppattern in $patterns; do
            if [[ "$dep" =~ $deppattern ]]; then
                new_depname=${dep##*/}
                new_deppath="$graft_dir/$new_depname"
                rel_path=$(realpath --relative-to="$libdir" $new_deppath) 
                new_dep="$origin/$rel_path"
                echo "$libfile depends on $dep, relinking to $new_deppath ($new_dep)"
                $relink $dep $new_dep $libfile
                if [ ! -f $new_deppath ]; then
                    echo "$new_deppath not found, copying and calling self" 
                    cp $dep $new_deppath
                    chmod 777 $new_deppath
                    graft_libs $new_deppath $graft_dir $patterns
                else
                    echo "grafted library $new_deppath already exists"
                fi
            fi
        done    
    done
}

function osx_relink {
    install_name_tool -change $1 $2 $3
}

function linux_relink {
    # local depname=${2#*/}
    local dep_dir=${2%/*}
    patchelf --set-rpath $dep_dir $3
}

cd $(dirname "$0")

cd roboschool/cpp-household
# make clean
make -j4
cd ..


graft_libs cpp_household.so .libs \
    ^/.+/libboost_python.+ \
    ^/.+Qt.+ \
    ^/.+/libassimp.+ \
    ^/.+/libLinearMath.+ \
    ^/.+/libBullet.+ \
    ^/.+/libPhysicsClientC_API.+ \
    ^/.+/libjpeg.+ \
    ^/.+/libpng.+ \
    ^/.+/libicu.+ \
    ^/.+/libdouble-conversion.+ \
    ^/.+/libminizip.+

if [ $(uname) == 'Darwin' ]; then
    # HACK - this should auto-detect plugins dir
    cp -r /usr/local/Cellar/qt/5.10.1/plugins .qt_plugins
    lib_pattern="*.dylib"
fi 
if [ $(uname) == 'Linux' ]; then
    cp -r /usr/lib/x86_64-linux-gnu/qt5/plugins .qt_plugins
    lib_pattern="*.so*"
fi
for lib in $(find .qt_plugins -name "$lib_pattern"); do 
     graft_libs $lib .libs ^/.+Qt.+ \
                ^/.+/libjpeg.+ \
                ^/.+/libpng.+ \
                ^/.+/libicu.+ \
                ^/.+/libdouble-conversion.+ \
                ^/.+/libminizip.+ \
                ^/.+/libxcb.+ \
                ^/.+/libxkb.+
done

