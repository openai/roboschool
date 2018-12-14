#!/bin/bash

git clone https://github.com/matthew-brett/multibuild && cd multibuild && git checkout 254ad28 && cd ..
. multibuild/common_utils.sh
. multibuild/travis_steps.sh
before_install

brew install qt assimp
./install_prereq.sh
pip wheel --no-deps -w wheelhouse

