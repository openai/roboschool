#!/bin/bash
set -ex
apt-get update && apt-get install -y libgl1-mesa-dev qtbase5-dev libqt5opengl5-dev libassimp-dev
pip install /io/roboschool*.whl

python -c "import roboschool, gym; gym.make('RoboschoolAnt-v1')"

