#!/bin/bash
#set -x
set -e

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` comp3d5_appimage"
  exit $E_BADARGS
fi

if [ ! -d distrib ]
then
  echo "Error ! Must be run from Comp3D base directory"
  exit $E_BADARGS
fi

if [ ! -f $1 ]
then
 echo "Error ! $1 does not exist."
 exit $E_BADARGS
fi

base=$(pwd)
echo "base dir: $base"

version=`cat src/compile.h | grep "COMP3D_VERSION" | sed 's/.*COMP3D v//' | sed 's/[^0-9.a-zA-Z_].*//'`
echo "version : $version"

mkdir -p distrib/comp3d5v$version/DEBIAN/
echo "Package: comp3d5v$version" > distrib/comp3d5v$version/DEBIAN/control
echo -n "Version: " >> distrib/comp3d5v$version/DEBIAN/control
echo $version >> distrib/comp3d5v$version/DEBIAN/control
echo "Section: base" >> distrib/comp3d5v$version/DEBIAN/control
echo "Priority: optional" >> distrib/comp3d5v$version/DEBIAN/control
echo "Architecture: all" >> distrib/comp3d5v$version/DEBIAN/control
echo "Depends: " >> distrib/comp3d5v$version/DEBIAN/control
echo "Maintainer: JM Muller <jean-michael.muller@ign.fr>" >> distrib/comp3d5v$version/DEBIAN/control
echo "Description: Micro-geodesic compensation software" >> distrib/comp3d5v$version/DEBIAN/control
echo "Homepage: http://www.ign.fr" >> distrib/comp3d5v$version/DEBIAN/control

cp distrib/postinst distrib/comp3d5v$version/DEBIAN/
cp distrib/postrm distrib/comp3d5v$version/DEBIAN/

rm -Rf distrib/comp3d5v$version/usr/bin/*
mkdir -p distrib/comp3d5v$version/usr/bin
cp $1 distrib/comp3d5v$version/usr/bin/comp3d5v$version

rm -rf distrib/comp3d5v$version/usr/share/pixmaps/*
mkdir -p distrib/comp3d5v$version/usr/share/pixmaps
cp data/comp3d5.xpm distrib/comp3d5v$version/usr/share/pixmaps/comp3d5v$version.xpm
mkdir -p distrib/comp3d5v$version/usr/share/applications

distrib/make_shortcut.sh
cp distrib/comp3d5.desktop distrib/comp3d5v$version/usr/share/applications/comp3d5v$version.desktop

cd distrib

rm -rf ./comp3d5v$version.deb

chmod -R 775 ./comp3d5v$version

dpkg-deb --build comp3d5v$version
echo "Final name: comp3d5v${version}.deb"
cd -

dir_name=comp3d_${version}_linux_deb
rm -f $base/${dir_name}.tgz
mkdir -p distrib/tmp/
cd distrib/tmp/
  rm -rf $dir_name
  mkdir $dir_name
cd -
  cp $base/distrib/comp3d5v${version}.deb distrib/tmp/$dir_name
cd -
  mkdir -p $dir_name/datasets
  mkdir -p $dir_name/data
  mkdir -p $dir_name/licenses
  cp -R $base/datasets/georef $dir_name/datasets/georef
  cp -R $base/datasets/gettingstarted $dir_name/datasets/gettingstarted
  cp -R $base/datasets/gettingfurther $dir_name/datasets/gettingfurther
  cp $base/LICENSE.md $base/README.md $base/LISEZMOI.md $base/CONTRIBUTORS.md $base/changelog.txt $dir_name
  cp $base/src/*/LICENSE* $base/gui/html/LICENSE* $base/NOTICE.md $dir_name/licenses
  cp $base/data/logo_* $dir_name/data
  tar zcvf $base/${dir_name}.tgz $dir_name
cd -



echo "Ok!"

cd distrib/tmp/ && rm -rf $dir_name
cd -
cd distrib/ && rm -rf comp3d5v$version/

