#!/bin/bash
#set -x
set -e

if [ $# -ne 2 ]
then
  echo "Usage: `basename $0` comp3d5_build_dir <doc_subdir>"
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

dir_name=comp3d_${version}_win

echo "dir_name: " $dir_name
rm -f $base/${dir_name}.zip
cd $1
  rm -rf $dir_name
  mkdir $dir_name
  ${base}/distrib/install_doc.sh ${base}/doc_uni $dir_name/$2
  cp -R release/proj $dir_name
  cp release/Comp3D.exe $dir_name
  mkdir -p $dir_name/datasets
  mkdir -p $dir_name/data
  mkdir -p $dir_name/licenses
  cp -R $base/datasets/georef $dir_name/datasets/georef
  cp -R $base/datasets/gettingstarted $dir_name/datasets/gettingstarted
  cp -R $base/datasets/gettingfurther $dir_name/datasets/gettingfurther
  cp $base/LICENSE.md $base/README.md $base/LISEZMOI.md $base/CONTRIBUTORS.md $base/changelog.txt $dir_name
  cp $base/src/*/LICENSE* $base/gui/html/LICENSE* $base/NOTICE.md $dir_name/licenses
  cp $base/data/logo_* $dir_name/data
  cp $base/data/comp3d5.xpm $dir_name
  zip -r $base/${dir_name}.zip $dir_name
cd -

echo "Zip created: " $base/${dir_name}.zip


