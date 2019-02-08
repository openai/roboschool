#!/bin/bash

git clone https://github.com/matthew-brett/multibuild && cd multibuild && git checkout 254ad28 && cd ..
. multibuild/common_utils.sh
. multibuild/travis_steps.sh
before_install

brew install qt assimp boost-python3 || echo "warning - brew returned non-zero exit code. \nThis may be a sign of error, but may also be because brew wants to point something out. Please inspect logs above"

. ./exports.sh
./install_bullet.sh
./roboschool_compile_and_graft.sh
pip wheel --no-deps -w wheelhouse .

