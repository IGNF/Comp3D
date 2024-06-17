#! /bin/bash

set -e
set -x
umask 0000
export PATH=/usr/lib/mxe/usr/bin/:$PATH

NBRP=$(cat /proc/cpuinfo | grep processor | wc -l)
GREEN='\033[0;32m'
STOP='\033[0m'


echo -e "${GREEN}Remove all${STOP}"
rm -Rf /tmp/AppDir

version=`cat /c3d/src/compile.h | grep "COMP3D_VERSION" | sed 's/.*COMP3D v//' | sed 's/[^0-9.a-zA-Z_].*//'`

COMP3D_DOC_DIR="/usr/share/doc/comp3d/${version}"

echo -e "${GREEN}Copy AppImage tools...${STOP}"
cp /linuxdeploy-x86_64.AppImage /c3d/
cp /linuxdeploy-plugin-qt-x86_64.AppImage /c3d/

echo -e "${GREEN}Creating AppImage...${STOP}"
cd /c3d/
distrib/make_appimage.sh  /c3d/autobuild/ /tmp/AppDir "${COMP3D_DOC_DIR}"
distrib/make_ziplinux.sh $(ls -t Comp3d5v5.*.AppImage | head -1)

echo -e "${GREEN}Creating deb package...${STOP}"
cd /c3d/
distrib/make_appimagedeb.sh $(ls -t Comp3d5v5.*.AppImage | head -1)
mv $(ls -t distrib/comp3d5v5.*.deb | head -1) .

COMP3D_DOC_DIR="doc"

echo -e "${GREEN}Creating windows zip...${STOP}"
cd /c3d/
distrib/make_zipwin.sh autobuild-mxe/ "${COMP3D_DOC_DIR}"

echo -e "${GREEN}Cleaning...${STOP}"
rm -Rf /c3d/autobuild-mxe/comp3d*_win /tmp/AppDir

echo -e "${GREEN}Done!${STOP}"
