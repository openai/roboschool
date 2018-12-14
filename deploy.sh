# !/bin/bash
set -ex
# ** HACK - rename linux wheels to manylinux1 wheels
cd wheelhouse
for whl in $(ls roboschool-*linux*.whl); do
    echo "HACK!!! Renaming $whl --> ${whl%linux_x86_64.whl}manylinux1_x86_64.whl"
    mv $whl ${whl%linux_x86_64.whl}manylinux1_x86_64.whl
done 
cd ..
# ** END HACK

if [[ ! -z "$TRAVIS_TAG" ]]; then
    pip install twine
    twine upload wheelhouse/*

    if [[ ! -z "$DEPLOY_SDIST" ]]; then
        python setup.py sdist
        twine upload dist/*       
    fi
fi

