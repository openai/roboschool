set -ex
cat $GCS_CREDS | base64 --decode > .gcs_creds.json
export GOOGLE_APPLICATION_CREDENTIALS=$PWD/.gcs_creds.json
gsutil cp wheelhouse/* gs://games-wheels/roboschool/
if [[ ! -z "$TRAVIS_TAG" ]]; then
    # pip install twine
    # twine upload wheelhouse/*

    if [[ ! -z "$DEPLOY_SDIST" ]]; then
        python setup.py sdist
        twine upload dist/*       
    fi
fi

