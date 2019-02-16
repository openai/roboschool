#!/bin/bash
set -ex
apt-get update && apt-get install -y libgl1-mesa-dev qtbase5-dev libqt5opengl5-dev libassimp-dev patchelf cmake > /dev/null

cd $(dirname "$0")
. ./exports.sh

./install_boost.sh
./install_bullet.sh
./roboschool_compile_and_graft.sh

pip wheel --no-deps -w /io/wheelhouse .


