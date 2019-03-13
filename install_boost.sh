#!/bin/bash
if [ $(uname) == 'Linux' ]; then
    source ./exports.sh
    BOOST_SRCDIR=$HOME/.boost_src
    mkdir -p $BOOST_SRCDIR && cd $BOOST_SRCDIR
    # curl -OL https://storage.googleapis.com/games-src/boost/boost_1_58_0.tar.bz2
    curl -OL https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz
    tar -xf boost_1_69_0.tar.gz
    cd boost_1_69_0

     ./bootstrap.sh --with-python=$(which python) --with-libraries=python --prefix=$HOME/.boost --with-python-root=$PYTHON_ROOT
     ./b2 install > $BOOST_SRCDIR/boost_make.log || (tail -100 $BOOST_SRCDIR/boost_make.log; exit 1)
fi
