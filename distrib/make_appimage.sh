#!/bin/bash
#set -x
set -e

if [ $# -ne 3 ]
then
  echo "Usage: `basename $0` comp3d5_build_dir comp_install_path doc_install_path"
  exit $E_BADARGS
fi

if [ ! -d distrib ]
then
  echo "Error ! Must be run from Comp3D base directory"
  exit $E_BADARGS
fi

if [ ! -d $1 ]
then
 echo "Error ! $1 is not a dir."
 exit $E_BADARGS
fi


base=$(pwd)
echo "base dir: $base"
version=`cat src/compile.h | grep "COMP3D_VERSION" | sed 's/.*COMP3D v//' | sed 's/[^0-9.a-zA-Z_].*//'`
echo "version : $version"
distrib/make_shortcut.sh

cd $1
  rm -Rf $2/
  mkdir -p $2
  cmake -DINSTALL_PREFIX="$2" ..
  make install
  $base/distrib/install_doc.sh ${base}/doc_uni $2/$3
  mkdir -p $2/usr/share/applications/
  mkdir -p $2/usr/share/pixmaps/
  mkdir -p $2/usr/local/proj93/share/proj/
  cp $2/usr/bin/Comp3D $2/usr/bin/comp3d5v$version
  cp -r /usr/local/proj93/share/proj/* $2/usr/local/proj93/share/proj/
  cp $base/distrib/comp3d5.desktop $2/usr/share/applications/comp3d5v$version.desktop
  cp $base/data/comp3d5.xpm $2/usr/share/pixmaps/comp3d5v$version.xpm
cd -
./linuxdeploy-x86_64.AppImage --appdir $2 --output appimage --plugin qt -v0

echo "Ok!"


