#!/bin/bash
set -ex
. multibuild/common_utils.sh
. multibuild/travis_steps.sh
before_install

pip install wheelhouse/roboschool*.whl

python -c "import roboschool, gym; gym.make('RoboschoolAnt-v1')"

