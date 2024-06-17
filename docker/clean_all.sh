#! /bin/bash

export PATH=/usr/lib/mxe/usr/bin/:$PATH

(
cd /c3d/
rm -Rf autobuild/
)&&(
cd /c3d/
rm -Rf autobuild-mxe/
)&&(
cd /c3d/tests/
rm -Rf autobuild/
rm -Rf autobuild-mxe/
)

