#! /bin/bash

set -x
cd $(dirname "$0")

. ./exports.sh

pip install cmake

if [ $(uname) == 'Linux' ]; then
    QT5_BIN=qt5.6.0_centos5_2018-12-11.tar.gz
    cd $CPP_HOUSEHOLD
    curl -OL https://storage.googleapis.com/games-src/qt5/$QT5_BIN
    tar -xf $QT5_BIN
    rm -rf $QT5_BIN

    BOOST_SRCDIR=$TMPDIR/boost
    mkdir -p $BOOST_SRCDIR && cd $BOOST_SRCDIR
    curl -OL https://storage.googleapis.com/games-src/boost/boost_1_58_0.tar.bz2
    tar -xf boost_1_58_0.tar.bz2
    cd boost_1_58_0

     ./bootstrap.sh --with-python=$(which python) --with-libraries=python 
     ./b2 install > $TMPDIR/boost_make.log || tail -100 $TMPDIR/boost_make.log

    ASSIMP_SRCDIR=$TMPDIR/assimp
    mkdir -p $ASSIMP_SRCDIR && cd $ASSIMP_SRCDIR
    curl https://codeload.github.com/assimp/assimp/tar.gz/v4.1.0 -o assimp-4.1.0.tar.gz
    tar -xf assimp-4.1.0.tar.gz
    cd assimp-4.1.0
    cmake . 
    make -j4 > $TMPDIR/assimp_make.log || tail -100 $TMPDIR/assimp_make.log
    make install 
    cd $ROBOSCHOOL_PATH

fi

if [ $(uname) == 'Darwin' ]; then
    brew install qt boost-python3 assimp
    pkg-config --libs Qt5Widgets Qt5OpenGL
    ls /usr/local/opt/qt/lib
fi

BULLET_SRCDIR=$TMPDIR/bullet3
rm -rf $BULLET_SRCDIR
mkdir -p $BULLET_SRCDIR && cd $BULLET_SRCDIR
git clone https://github.com/olegklimov/bullet3 -b roboschool_self_collision .

if [ $(uname) == 'Darwin' ]; then sed -i '' 's/SET CMP0042 NEW/SET CMP0042 OLD/g' CMakeLists.txt; fi

mkdir build && cd build
# cmake -DBUILD_SHARED_LIBS=ON -DUSE_DOUBLE_PRECISION=1 -DCMAKE_INSTALL_PREFIX:PATH=$ROBOSCHOOL_PATH/roboschool/cpp-household/bullet_local_install -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..
cmake -DBUILD_SHARED_LIBS=ON -DUSE_DOUBLE_PRECISION=1 -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..
make -j4 > $TMPDIR/bullet_make.log || tail -100 $TMPDIR/bullet_make.log
make install

ls $CPP_HOUSEHOLD/bullet_local_install/lib
cd $CPP_HOUSEHOLD && make -j4 

cd $ROBOSCHOOL_PATH
