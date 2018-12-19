#!/bin/bash

git clone https://github.com/matthew-brett/multibuild && cd multibuild && git checkout 254ad28 && cd ..
. multibuild/common_utils.sh
. multibuild/travis_steps.sh
before_install

brew install qt assimp boost-python3
. ./exports.sh
./install_bullet.sh
./roboschool_compile_and_graft.sh
pip wheel --no-deps -w wheelhouse .

