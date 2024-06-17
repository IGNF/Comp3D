#! /bin/bash

set -e
set -x
umask 0000
export PATH=/usr/lib/mxe/usr/bin/:$PATH

NBRP=$(cat /proc/cpuinfo | grep processor | wc -l)
GREEN='\033[0;32m'
STOP='\033[0m'
export CCACHE_DIR=/c3d/CCACHE_DOCKER

echo -e "${GREEN}Compile doc${STOP}"
cd /c3d/doc_uni
./build_doc.sh

echo -e "${GREEN}Remove all${STOP}"
rm -Rf /c3d/autobuild/ /c3d/autobuild-mxe/ /c3d/tests/autobuild/ /c3d/tests/autobuild-mxe/

version=`cat /c3d/src/compile.h | grep "COMP3D_VERSION" | sed 's/.*COMP3D v//' | sed 's/[^0-9.a-zA-Z_].*//'`

COMP3D_DOC_DIR="/usr/share/doc/comp3d/${version}"
echo -e "${GREEN}Linux compilation${STOP}"
cd /c3d/
lrelease Comp3D_cpp.pro
mkdir -p autobuild/
cd autobuild/
qmake ../Comp3D_cpp.pro CONFIG+=release COMP3D_DOC_DIR="${COMP3D_DOC_DIR}"
make clean
make -j$NBRP

COMP3D_DOC_DIR="doc"
echo -e "${GREEN}Windows cross-compilation${STOP}"
cd /c3d/
mkdir -p autobuild-mxe/
cd autobuild-mxe/
x86_64-w64-mingw32.static-qmake-qt5 ../Comp3D_cpp.pro COMP3D_DOC_DIR="${COMP3D_DOC_DIR}"
make clean
make -j$NBRP
cp -R /usr/local/proj82/share/proj/ release/proj

echo -e "${GREEN}Compile tests on linux...${STOP}"
cd /c3d/tests/
mkdir -p autobuild/
cd autobuild/
qmake ../tests.pro
make clean
make -j$NBRP
cd ..
echo -e "${GREEN}Run tests on linux...${STOP}"
autobuild/Comp3D_tests
echo -e "${GREEN}Linux tests finished, no errors.${STOP}"

echo -e "${GREEN}Compile tests on wine...${STOP}"
cd /c3d/tests/
mkdir -p autobuild-mxe/
cd autobuild-mxe/
x86_64-w64-mingw32.static-qmake-qt5 ../tests.pro
make clean
make -j$NBRP
cd ..
echo -e "${GREEN}Run tests on wine...${STOP}"
mkdir /tmp/wineprefix
export WINEPREFIX=/tmp/wineprefix/
wine autobuild-mxe/release/Comp3D_tests.exe
echo -e "${GREEN}Wine tests finished, no errors.${STOP}"

#to see all messages on wine tests outside docker, run:
#wine start /wait autobuild-mxe/release/Comp3D_tests.exe
