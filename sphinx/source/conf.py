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
output_dir = 'dox'
subprocess.call('doxygen', shell=True)
breathe_projects['udho'] = output_dir + '/xml'

exhale_args = {
    # These arguments are required
    "containmentFolder":     "./api",
    "rootFileName":          "root.rst",
    "rootFileTitle":         "Udho API",
    "doxygenStripFromPath":  "..",
    # Suggested optional arguments
    "createTreeView":        True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin":    "INPUT = ../../includes"
}

# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'cpp'

breathe_projects_source = {
    "udho" : ( "../includes/udho", [
        "access.h", "activities.h", "application.h", "attachment.h", 
        "bridge.h", 
        "cache.h", "client.h", "compositors.h", "configuration.h", "connection.h", "context.h", "contexts.h", "cookie.h", 
        "defs.h", 
        "form.h", 
        "listener.h", "logging.h", 
        "page.h", "parser.h", 
        "router.h", 
        "scope.h", "server.h", "session.h", "sse.h", 
        "url.h", "util.h",
        "visitor.h",
        "watcher.h"
    ])
}

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
# html_theme = 'alabaster'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

if not read_the_docs_build:
    import sphinx_rtd_theme
    html_theme = 'sphinx_rtd_theme'
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
