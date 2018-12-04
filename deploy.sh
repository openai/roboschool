set -ex
if [[ ! -z "$TRAVIS_TAG" ]]; then
    pip install twine
    twine upload wheelhouse/*

    if [[ ! -z "$DEPLOY_SDIST" ]]; then
        python setup.py sdist
        twine upload dist/*       
    fi
fi

