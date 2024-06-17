#!/bin/bash
#set -x
set -e

if [ ! -d distrib ]
then
  echo "Error ! Must be run from Comp3D base directory"
  exit $E_BADARGS
fi

version=`cat src/compile.h | grep "COMP3D_VERSION" | sed 's/.*COMP3D v//' | sed 's/[^0-9.a-zA-Z_].*//'`
echo "version : $version"

echo "[Desktop Entry]" > distrib/comp3d5.desktop
echo "Version=1.0" >> distrib/comp3d5.desktop
echo "Name=Comp3d5v$version" >> distrib/comp3d5.desktop
echo "Type=Application" >> distrib/comp3d5.desktop
echo "Comment=Micro-geodesic compensation software" >> distrib/comp3d5.desktop
echo "Exec=comp3d5v$version" >> distrib/comp3d5.desktop
echo "Icon=comp3d5v$version" >> distrib/comp3d5.desktop
echo "Terminal=false" >> distrib/comp3d5.desktop
echo "Categories=Education;Science;" >> distrib/comp3d5.desktop

