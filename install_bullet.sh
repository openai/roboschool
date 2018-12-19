pip install cmake
BULLET_SRCDIR=$TMPDIR/bullet3
rm -rf $BULLET_SRCDIR
mkdir -p $BULLET_SRCDIR && cd $BULLET_SRCDIR
git clone https://github.com/olegklimov/bullet3 -b roboschool_self_collision .

mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON -DUSE_DOUBLE_PRECISION=1 -DCMAKE_INSTALL_PREFIX:PATH=$TMPDIR/bullet_local_install -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..

make -j4 > $TMPDIR/bullet_make.log || (tail -100 $TMPDIR/bullet_make.log; exit 1)
make install > $TMPDIR/bullet_install.log || (tail -100 $TMPDIR/bullet_install.log; exit 1)

if [ $(uname) == 'Darwin' ]; then
    
    for lib in $(find $TMPDIR/bullet_local_install/lib -name "*.dylib"); do
        install_name_tool -id $lib $lib
        for dep in $(otool -L $lib | grep "@rpath" | awk '{print $1}'); do 
            install_name_tool -change $dep "$TMPDIR/bullet_local_install/lib/${dep##@rpath/}" $lib
        done
    done
fi
if [ $(uname) == 'Linux' ]; then
    
    for lib in $(find $TMPDIR/bullet_local_install/lib -name "*.so.2.87"); do
        patchelf --set-rpath $TMPDIR/bullet_local_install/lib $lib
    done
fi

