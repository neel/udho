# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

import subprocess, os
from pathlib import Path

def configureDoxyfile(docs_in, docs_out, source_in):
    with open(str((docs_in / 'Doxyfile.in').absolute()), 'r') as file :
        filedata = file.read()

    filedata = filedata.replace('@CMAKE_CURRENT_SOURCE_DIR@', str(docs_in))
    filedata = filedata.replace('@CMAKE_CURRENT_BINARY_DIR@', str(docs_out))
    filedata = filedata.replace('@UDHO_SOURCE_DIR@',          str(source_in))

    with open('Doxyfile', 'w') as file:
        file.write(filedata)

read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

# -- Project information -----------------------------------------------------

project = 'udho'
copyright = '2020, Neel Basu (Sunanda Bose)'
author = 'Neel Basu (Sunanda Bose)'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ["breathe", "exhale", "sphinx.ext.autosectionlabel", 'sphinx.ext.graphviz']
breathe_projects = {}
breathe_default_project = "udho"
autosectionlabel_prefix_document = True
autosectionlabel_maxdepth = 3
primary_domain = 'cpp'
highlight_language = 'cpp'

d_source     = Path(os.path.dirname(os.path.realpath(__file__)))
d_docs       = d_source.parent
d_udho       = d_docs.parent
d_build      = d_udho / 'build'
d_build_docs = d_build / 'docs'

configureDoxyfile(d_docs.absolute(), d_build_docs.absolute(),  d_udho.absolute())
breathe_projects['udho'] = str(d_build_docs.absolute() / 'xml')
subprocess.call('doxygen', shell=True)

exhale_args = {
    "containmentFolder":     str((d_build_docs / 'sphinx' / 'api').absolute()),
    "rootFileName":          str((d_build_docs / 'sphinx' / 'root.rst').absolute()),
    "rootFileTitle":         "Udho API",
    "doxygenStripFromPath":  "..",
    "createTreeView":        True
}

if not read_the_docs_build:
    import sphinx_rtd_theme
    html_theme = 'sphinx_rtd_theme'
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]