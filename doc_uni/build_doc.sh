#! /bin/bash
rm -Rf _build/ en/ fr/ tmp/
sphinx-build -M html . en -Dlanguage='en'
#sphinx-build -M html . fr -Dlanguage='fr'

