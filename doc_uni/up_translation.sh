#! /bin/bash
touch index.rst
sphinx-build -M gettext . _build
sphinx-intl update -p _build/gettext -l fr
