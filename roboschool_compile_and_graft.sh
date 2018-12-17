#!/bin/bash
set -e

function graft_libs {
    local libfile=$1
    local libdir=$(dirname $libfile)
    if [ $(uname) == 'Darwin' ]; then
        local grafter="install_name_tool -change" 
        local library_lister="otool -L"
    fi
    if [ $(uname) == 'Linux' ]; then
        local grafter="patchelf --replace-needed" 
        local library_lister="ldd"
    fi

    local deps=$($library_lister $libfile | awk 'FNR>2 {print $1}')
    local patterns=${@:3}
    
    graft_dir=$2
    mkdir -p $graft_dir

    for dep in $deps; do
        echo $dep
        for deppattern in $patterns; do
            if [[ "$dep" =~ $deppattern ]]; then
                new_depname=${dep##*/}
                new_deppath="$graft_dir/$new_depname"
                rel_path=$(realpath --relative-to="$libdir" $new_deppath) 
                new_dep="@loader_path/$rel_path"
                echo "$libfile depends on $dep, relinking to $new_deppath ($new_dep)"
                $grafter $dep $new_dep $libfile
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

cd $(dirname "$0")

cd roboschool/cpp-household
make clean
make -j4
cd ..

# graft_libs cpp_household.so .libs ^/.+/Python
graft_libs cpp_household.so .libs ^/.+/libboost_python.+
graft_libs cpp_household.so .libs ^/.+/Qt.+
graft_libs cpp_household.so .libs ^/.+/libassimp.+
graft_libs cpp_household.so .libs ^/.+/libLinearMath.+ ^/.+/libBullet.+ ^/.+/libPhysicsClientC_API.+

if [ $(uname) == 'Darwin' ]; then
    # HACK - this should auto-detect plugins dir
    cp -r /usr/local/Cellar/qt/5.12.0/plugins .qt_plugins

    for lib in $(find .qt_plugins -name "*.dylib"); do 
         graft_libs $lib .libs ^/.+/Qt.+
    done
   
fi 
# if [ $(uname) == 'Linux' ]; then
#    cp -r 


