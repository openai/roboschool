#! /bin/bash

set -x
cd $(dirname "$0")

ROBOSCHOOL_PATH=$(pwd)
CPP_HOUSEHOLD=$ROBOSCHOOL_PATH/roboschool/cpp-household
pip install cmake

if [ $(uname) == 'Linux' ]; then
  QT5_BIN=qt5.6.0_centos5_2018-12-06.tar.gz
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
export CPATH=$CPATH:/usr/local/include/python3.6m
PYTHON=$(which python)
PYTHON_BIN=$(readlink -f $PYTHON) 
ln -sf $PYTHON_BIN ${PYTHON_BIN}m
PYTHON_ROOT=${PYTHON%/bin/python}
 ./bootstrap.sh --prefix=$ROBOSCHOOL_PATH/roboschool/cpp-household/boost_local_install --with-python=$PYTHON --with-python-root=$PYTHON_ROOT --with-libraries=python 
 ./b2 install > $TMPDIR/boost_make.log
tail -100 $TMPDIR/boost_make.log

ASSIMP_SRCDIR=$TMPDIR/assimp
mkdir -p $ASSIMP_SRCDIR && cd $ASSIMP_SRCDIR
curl https://codeload.github.com/assimp/assimp/tar.gz/v4.1.0 -o assimp-4.1.0.tar.gz
tar -xf assimp-4.1.0.tar.gz
cd assimp-4.1.0
cmake -DCMAKE_INSTALL_PREFIX:PATH=$CPP_HOUSEHOLD/assimp_local_install . 
make -j4 
make install > /dev/null
cd $ROBOSCHOOL_PATH

BULLET_SRCDIR=$TMPDIR/bullet3
rm -rf $BULLET_SRCDIR
mkdir -p $BULLET_SRCDIR && cd $BULLET_SRCDIR
git clone https://github.com/olegklimov/bullet3 -b roboschool_self_collision .
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON -DUSE_DOUBLE_PRECISION=1 -DCMAKE_INSTALL_PREFIX:PATH=$ROBOSCHOOL_PATH/roboschool/cpp-household/bullet_local_install -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..
make -j4
make install > /dev/null

cd $ROBOSCHOOL_PATH
pip wheel .
