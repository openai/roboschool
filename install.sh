#! /bin/bash

set -e
cd $(dirname "$0")

ROBOSCHOOL_PATH=$(pwd)
CPP_HOUSEHOLD=$ROBOSCHOOL_PATH/roboschool/cpp-household
pip install cmake

QT5_SRCDIR=$TMPDIR/qt5
QT_SRC=qt-everywhere-opensource-src-5.6.0.tar.gz
mkdir -p $QT5_SRCDIR && cd $QT5_SRCDIR
# curl -OL https://storage.googleapis.com/games-src/qt5/qtbase-opensource-src_5.7.1+dfsg.orig.tar.bz2
if [ ! -f $QT_SRC ]; then curl -OL http://download.qt.io/archive/qt/5.6/5.6.0/single/$QT_SRC; fi
tar -xf $QT_SRC
cd ${QT_SRC%.tar.gz}

if [ $(uname) == 'Linux' ]; then
    # Tweak Qt code a little bit (build errors without these):
    # 1. remove reference to signalfd.h not present on CentOS 5
    sed -i '/signalfd/d' qtbase/src/platformsupport/fbconvenience/qfbvthandler.cpp
    # 2. remove reference to asm/byteorder.h - present, but fails to compile with --std=c++0x
    sed -i '/byteorder/d' qtbase/src/testlib/3rdparty/linux_perf_event_p.h
    # 3. add legacy glx flag to the compiler
    echo "QMAKE_CXXFLAGS += -DGLX_GLXEXT_LEGACY" >> qtbase/src/plugins/platforms/offscreen/offscreen.pro
fi

./configure -opensource -confirm-license -prefix $CPP_HOUSEHOLD/qt5_local_install -widgets -opengl -make libs \
             -no-gstreamer -no-pulseaudio -no-alsa \
             -no-securetransport -no-openssl -no-libproxy \
             -no-xcb -no-harfbuzz -no-mtdev -no-freetype -no-gtkstyle \
             -no-xinput2 -no-libinput

cat qtbase/src/plugins/platforms/offscreen/offscreen.pro

make -j4 > /qt5_build.log
tail -500 /qt5_build.log
make install > /dev/null

ASSIMP_SRCDIR=$TMPDIR/assimp
mkdir -p $ASSIMP_SRCDIR && cd $ASSIMP_SRCDIR
curl https://codeload.github.com/assimp/assimp/tar.gz/v4.1.0 -o assimp-4.1.0.tar.gz
tar -xf assimp-4.1.0.tar.gz
cd assimp-4.1.0
cmake -DCMAKE_INSTALL_PREFIX:PATH=$CPP_HOUSEHOLD/assimp_local_install . 
make -j4
make install > /dev/null
cd $ROBOSCHOOL_PATH

BOOST_SRCDIR=$TMPDIR/boost
mkdir -p $BOOST_SRCDIR && cd $BOOST_SRCDIR
curl -OL https://storage.googleapis.com/games-src/boost/boost_1_58_0.tar.bz2
tar -xf boost_1_58_0.tar.bz2
cd boost_1_58_0
export CPATH=$CPATH:/usr/local/include/python3.6m
 ./bootstrap.sh --prefix=$ROBOSCHOOL_PATH/roboschool/cpp-household/boost_local_install --with-python=$(which python) --with-libraries=python
 ./b2 install > /dev/null

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
