# !/bin/bash
cd wheelhouse
# ** HACK - rename linux wheels to manylinux1 wheels
# for f in "roboschool-*linux*.whl"; do
#     mv sss
# done 

if [[ ! -z "$TRAVIS_TAG" ]]; then
    pip install twine
    twine upload wheelhouse/*

    if [[ ! -z "$DEPLOY_SDIST" ]]; then
        python setup.py sdist
        twine upload dist/*       
    fi
fi

