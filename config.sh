#!/bin/bash
function pre_build {
    set -ex
    ./install.sh
}

function run_tests {
    python -c "import roboschool; gym.make('RoboschoolAnt-v1')"
}

