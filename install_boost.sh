#!/bin/bash

if [ $(uname) == 'Linux' ]; then
    BOOST_SRCDIR=$TMPDIR/boost
    mkdir -p $BOOST_SRCDIR && cd $BOOST_SRCDIR
    curl -OL https://storage.googleapis.com/games-src/boost/boost_1_58_0.tar.bz2
    tar -xf boost_1_58_0.tar.bz2
    cd boost_1_58_0

     # ./bootstrap.sh --with-python=$(which python) --with-libraries=python 
     ./bootstrap.sh --with-python=$(which python) --with-libraries=python
     ./b2 install > $TMPDIR/boost_make.log || (tail -100 $TMPDIR/boost_make.log; exit 1)
fi
