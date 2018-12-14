set +x
echo $GCS_CREDS | base64 --decode > $PWD/.gcs_creds.json
wc -l .gcs_creds.json
set -ex
export GOOGLE_APPLICATION_CREDENTIALS=$PWD/.gcs_creds.json
gsutil cp setup.py gs://games-wheels/roboschool/

