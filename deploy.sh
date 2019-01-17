# !/bin/bash
set -ex
# ** HACK - rename linux wheels to manylinux1 wheels
if [ $(uname) == 'Linux' ]; then
    cd wheelhouse
    for whl in $(ls roboschool-*linux*.whl); do
        echo "HACK!!! Renaming $whl --> ${whl%linux_x86_64.whl}manylinux1_x86_64.whl"
        sudo mv $whl ${whl%linux_x86_64.whl}manylinux1_x86_64.whl
    done 
    cd ..
    pip install --user bleach==3.0.2
    pip install --user twine==1.12.1
fi
# ** END HACK

if [ $(uname) == 'Darwin' ]; then
    export PATH=$PWD/venv/bin:$PATH
    pip install twine
fi

if [[ ! -z "$TRAVIS_TAG" ]]; then
    python -m twine upload wheelhouse/*

    if [[ ! -z "$DEPLOY_SDIST" ]]; then
        python setup.py sdist
        python -m twine upload dist/*       
    fi
fi

