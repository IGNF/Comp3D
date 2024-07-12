rmdir /Q /S _build en fr tmp
sphinx-build -M html . en -Dlanguage='en'
rem sphinx-build -M html . fr -Dlanguage='fr'
