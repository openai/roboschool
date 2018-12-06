#!/bin/bash
function pre_build {
    set -ex
    . ./exports.sh
    env
    ./install.sh
    env
}

function run_tests {
    python -c "import roboschool; gym.make('RoboschoolAnt-v1')"
}

