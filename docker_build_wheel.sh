#!/bin/bash

apt-get update && apt-get install -y libgl1-mesa-dev qtbase5-dev libqt5opengl5-dev assimp

. ./exports.sh
./install_prereq.sh

pip wheel --no-deps -w /io/wheelhouse .


