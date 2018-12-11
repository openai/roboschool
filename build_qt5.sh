set -ex
# Run in a centos5 / manylinux docker image
QT5_SRCDIR=$TMPDIR/qt5
QT_SRC=qt-everywhere-opensource-src-5.6.0.tar.gz
mkdir -p $QT5_SRCDIR && cd $QT5_SRCDIR
if [ ! -f $QT_SRC ]; then curl -OL http://download.qt.io/archive/qt/5.6/5.6.0/single/$QT_SRC; fi
tar -xf $QT_SRC
cd ${QT_SRC%.tar.gz}

# possibly make a diff patch out of these
if [ $(uname) == 'Linux' ]; then
    # Tweak Qt code a little bit (build errors without these):
    # 1. remove reference to signalfd.h not present on CentOS 5
    sed -i '/signalfd/d' qtbase/src/platformsupport/fbconvenience/qfbvthandler.cpp
    # 2. remove reference to asm/byteorder.h - present, but fails to compile with --std=c++0x
    sed -i '/byteorder/d' qtbase/src/testlib/3rdparty/linux_perf_event_p.h
    # 3. add legacy glx flag to the compiler
    echo "QMAKE_CXXFLAGS += -DGLX_GLXEXT_LEGACY" >> qtbase/src/plugins/platforms/offscreen/offscreen.pro
fi
# if [ $(uname) == 'Darwin' ]; then
#     # Tweak Qt code a little bit (build errors without these):
#     sed -i '' '/InvalidContext/d' qtbase/src/gui/painting/qcoregraphics.mm
#     sed -i '' '/InvalidBounds/d' qtbase/src/gui/painting/qcoregraphics.mm
#     sed -i '' '/InvalidImage/d' qtbase/src/gui/painting/qcoregraphics.mm
# fi

./configure -opensource -confirm-license -prefix $CPP_HOUSEHOLD/qt5_local_install -widgets -opengl -make libs \
             -no-gstreamer -no-pulseaudio -no-alsa \
             -no-securetransport -no-openssl -no-libproxy \
             -no-harfbuzz \
             -no-libinput -no-evdev

cat qtbase/src/plugins/platforms/offscreen/offscreen.pro

make -j4 > $TMPDIR/qt5_build.log || tail -500 $TMPDIR/qt5_build.log
make install > /dev/null


