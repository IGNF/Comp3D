#! /bin/bash
rm -Rf _build/ en/ fr/ tmp/
# make -e SPHINXOPTS="-Dlanguage='fr'" -e BUILDDIR="fr" html
make -e SPHINXOPTS="-Dlanguage='en'" -e BUILDDIR="en" html
# make -e SPHINXOPTS="-Dlanguage='en'" -e BUILDDIR="en" singlehtml
