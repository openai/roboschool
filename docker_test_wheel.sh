#!/bin/bash
set -ex
apt-get update && apt-get install -y libgl1-mesa-dev > /dev/null
pip install /io/wheelhouse/roboschool*.whl

python -c "import roboschool, gym; gym.make('RoboschoolAnt-v1')"

