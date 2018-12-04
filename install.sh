#! /bin/bash

set -ex
cd $(dirname "$0")
ROBOSCHOOL_PATH=$(pwd)
CPP_HOUSEHOLD=$ROBOSCHOOL_PATH/roboschool/cpp-household
pip install cmake

QT5_SRCDIR=$TMPDIR/qt5
mkdir -p $QT5_SRCDIR && cd $QT5_SRCDIR
curl -OL http://deb.debian.org/debian/pool/main/q/qtbase-opensource-src/qtbase-opensource-src_5.7.1+dfsg.orig.tar.xz
tar -xJf qtbase-opensource-src_5.7.1+dfsg.orig.tar.xz 
cd qtbase-opensource-src-5.7.1
./configure -opensource -confirm-license -prefix $CPP_HOUSEHOLD/qt5_local_install -qt-xcb
make && make install

ASSIMP_SRCDIR=$TMPDIR/assimp
mkdir -p $ASSIMP_SRCDIR && cd $ASSIMP_SRCDIR
curl https://codeload.github.com/assimp/assimp/tar.gz/v4.1.0 -o assimp-4.1.0.tar.gz
tar -xf assimp-4.1.0.tar.gz
cd assimp-4.1.0
cmake -DCMAKE_INSTALL_PREFIX:PATH=$CPP_HOUSEHOLD/assimp_local_install . 
make -j4
make install
cd $ROBOSCHOOL_PATH

BOOST_SRCDIR=$TMPDIR/boost
mkdir -p $BOOST_SRCDIR && cd $BOOST_SRCDIR
curl -OL https://storage.googleapis.com/games-src/boost/boost_1_58_0.tar.bz2
tar -xf boost_1_58_0.tar.bz2
cd boost_1_58_0
export CPATH=$CPATH:/usr/local/include/python3.6m
 ./bootstrap.sh --prefix=$ROBOSCHOOL_PATH/roboschool/cpp-household/boost_local_install --with-python=$(which python) --with-libraries=python
 ./b2 install

BULLET_SRCDIR=$TMPDIR/bullet3
rm -rf $BULLET_SRCDIR
mkdir -p $BULLET_SRCDIR && cd $BULLET_SRCDIR
git clone https://github.com/olegklimov/bullet3 -b roboschool_self_collision .
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON -DUSE_DOUBLE_PRECISION=1 -DCMAKE_INSTALL_PREFIX:PATH=$ROBOSCHOOL_PATH/roboschool/cpp-household/bullet_local_install -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..
make -j4
make install

cd $ROBOSCHOOL_PATH
pip wheel .
