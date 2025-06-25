# -*- coding: utf-8 -*-

from sphinx_intl import __version__

# -- Project information -----------------------------------------------------

project = 'Comp3D'
copyright = 'IGN'
author = 'IGN'

#version = release = __version__

# -- General configuration ---------------------------------------------------

source_suffix = '.rst'
master_doc = 'index'
language = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', '*-env']
pygments_style = 'sphinx'
source_encoding = 'utf-8'

extensions = ['sphinx.ext.mathjax', 'sphinx-mathjax-offline']

# -- Options for HTML output -------------------------------------------------

#html_theme = 'alabaster'

html_sidebars = {
    '**': [
        'about.html',
        'globaltoc.html',
        'searchbox.html',
    ]
}

# These folders are copied to the documentation's HTML output
html_static_path = ['_static']

# These paths are either relative to html_static_path
# or fully qualified paths (eg. https://...)
html_css_files = [
    'comp_doc.css',
]

# -- Options for sphinx-intl example

locale_dirs = ['locale/']   # po files will be created in this directory
gettext_compact = False     # optional: avoid file concatenation in sub directories.

extensions.append('sphinx.ext.todo')
todo_include_todos=True

# -- global rst -------------------------------------------------

comp_version = "v??"
with open ( "../src/compile.h" ) as F:
  for line in F.readlines():
    if "COMP3D_VERSION" in line:
      try:
        comp_version = line.split('"')[1].split()[-1]
        break
      except:
        print("Error getting Comp3D version from compile.h !")

rst_prolog = """
.. |c3| replace:: Comp3D
.. |c3_version| replace:: """+comp_version+"""
.. |gui| replace:: graphical user interface
.. |more| replace:: more details and explanations
.. |nc| replace:: /!\\\ NOT CLEAR
"""

