#!/bin/bash
set -ex
pip install wheelhouse/roboschool*.whl

python -c "import roboschool, gym; gym.make('RoboschoolAnt-v1')"

