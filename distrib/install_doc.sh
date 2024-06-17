#!/bin/bash
#set -x
set -e

if [ $# -ne 2 ]
then
  echo "Usage: `basename $0` <src_doc_path> <dest_doc_path>"
  exit $E_BADARGS
fi

for lang in  $1/* ; do
    [ ! -d ${lang}/html ] && continue
    dest=$2/$(basename ${lang})/
    mkdir -p ${dest}
    cp -R ${lang}/html ${dest}
done


echo "Doc installed in: $2"
