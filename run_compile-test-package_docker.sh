#!/bin/bash
set -e

TEST_ONLY=
case "$1" in
  "test") TEST_ONLY=1 ;;
  "") ;;
  *) echo "Usage : $0 [test]" && exit 0
esac

set -x
cd docker

echo "###### Build docker image ######"
docker build --network=host -t c3ddocker2004 .

echo "###### Clean autobuild directory ######"
docker run --rm --device /dev/fuse --privileged -v $(pwd)/..:/c3d c3ddocker2004 /c3d/docker/clean_all.sh

echo "###### Run and compile tests ######"
docker run --user "$(id -u):$(id -g)" --rm  -v $(pwd)/..:/c3d c3ddocker2004 /c3d/docker/compile_all.sh

[ "$TEST_ONLY" ] && exit 0

echo "###### Create package ######"
docker run --rm --device /dev/fuse --privileged -v $(pwd)/..:/c3d c3ddocker2004 /c3d/docker/package_all.sh

cd ..
