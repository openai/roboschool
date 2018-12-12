#!/bin/bash
function pre_build {
    set -ex
    . ./exports.sh
    env
    ./install.sh
    env
}

function run_tests {
    export DYLD_PRINT_LIBRARIES=1
    python -c "import roboschool; import gym; gym.make('RoboschoolAnt-v1')"
}

