#! /bin/bash

set -x
cd $(dirname "$0")

. ./exports.sh

ROBOSCHOOL_PATH=$(pwd)
CPP_HOUSEHOLD=$ROBOSCHOOL_PATH/roboschool/cpp-household
pip install cmake

if [ $(uname) == 'Linux' ]; then
  QT5_BIN=qt5.6.0_centos5_2018-12-06a.tar.gz
fi

cd $CPP_HOUSEHOLD
curl -OL https://storage.googleapis.com/games-src/qt5/$QT5_BIN
tar -xf $QT5_BIN
rm -rf $QT5_BIN

BOOST_SRCDIR=$TMPDIR/boost
mkdir -p $BOOST_SRCDIR && cd $BOOST_SRCDIR
curl -OL https://storage.googleapis.com/games-src/boost/boost_1_58_0.tar.bz2
tar -xf boost_1_58_0.tar.bz2
cd boost_1_58_0

 ./bootstrap.sh --prefix=$ROBOSCHOOL_PATH/roboschool/cpp-household/boost_local_install --with-python=$(which python) --with-libraries=python 
 ./b2 install > $TMPDIR/boost_make.log || tail -100 $TMPDIR/boost_make.log

ASSIMP_SRCDIR=$TMPDIR/assimp
mkdir -p $ASSIMP_SRCDIR && cd $ASSIMP_SRCDIR
curl https://codeload.github.com/assimp/assimp/tar.gz/v4.1.0 -o assimp-4.1.0.tar.gz
tar -xf assimp-4.1.0.tar.gz
cd assimp-4.1.0
cmake -DCMAKE_INSTALL_PREFIX:PATH=$CPP_HOUSEHOLD/assimp_local_install . 
make -j4 > $TMPDIR/assimp_make.log || tail -100 $TMPDIR/assimp_make.log
make install 
cd $ROBOSCHOOL_PATH

BULLET_SRCDIR=$TMPDIR/bullet3
rm -rf $BULLET_SRCDIR
mkdir -p $BULLET_SRCDIR && cd $BULLET_SRCDIR
git clone https://github.com/olegklimov/bullet3 -b roboschool_self_collision .
mkdir build && cd build
cmake -DUSE_DOUBLE_PRECISION=1 -DCMAKE_INSTALL_PREFIX:PATH=$ROBOSCHOOL_PATH/roboschool/cpp-household/bullet_local_install -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..
make -j4 > $TMPDIR/bullet_make.log || tail -100 $TMPDIR/bullet_make.log
make install

ls $CPP_HOUSEHOLD/bullet_local_install/lib
cd $CPP_HOUSEHOLD && make -j4 

cd $ROBOSCHOOL_PATH
pip wheel . -w wheelhouse
# cd wheelhouse
# unzip roboschool-1.0-cp35-cp35m-linux_x86_64.whl
# ls -lht roboschool/cpp-household/bullet_local_install/lib
# 
# auditwheel -v show roboschool-1.0-cp35-cp35m-linux_x86_64.whl
# auditwheel repair roboschool-1.0-cp35-cp35m-linux_x86_64.whl
# ls wheelhouse

