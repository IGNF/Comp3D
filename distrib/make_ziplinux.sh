#!/bin/bash
#set -x
set -e

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` comp3d_appimage"
  exit $E_BADARGS
fi

if [ ! -d distrib ]
then
  echo "Error ! Must be run from Comp3D base directory"
  exit $E_BADARGS
fi

if [ ! -f $1 ]
then
 echo "Error ! $1 is not a file."
 exit $E_BADARGS
fi

base=$(pwd)
echo "base dir: $base"
version=`cat src/compile.h | grep "COMP3D_VERSION" | sed 's/.*COMP3D v//' | sed 's/[^0-9.a-zA-Z_].*//'`
echo "version : $version"
distrib/make_shortcut.sh

dir_name=comp3d_${version}_linux

echo "appimage name: " $1
rm -f $base/${dir_name}.tgz
mkdir -p distrib/tmp/
cd distrib/tmp/
  rm -rf $dir_name
  mkdir $dir_name
cd -
  cp $base/$1 distrib/tmp/$dir_name
cd -
  mkdir -p $dir_name/datasets
  mkdir -p $dir_name/data
  mkdir -p $dir_name/licenses
  cp -R $base/datasets/georef $dir_name/datasets/georef
  cp -R $base/datasets/gettingstarted $dir_name/datasets/gettingstarted
  cp -R $base/datasets/gettingfurther $dir_name/datasets/gettingfurther
  cp $base/LICENSE.md $base/README.md $base/CONTRIBUTORS.md $base/changelog.txt $dir_name
  cp $base/src/*/LICENSE* $base/gui/html/LICENSE* $base/NOTICE.md $dir_name/licenses
  cp $base/data/logo_* $dir_name/data
  tar zcvf $base/${dir_name}.tgz $dir_name
cd -

echo "Tgz created: " $base/${dir_name}.tgz

cd distrib/tmp/ && rm -rf $dir_name

