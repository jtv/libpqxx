# Configuration file for the Sphinx documentation builder.

from os import getenv
from pathlib import Path


# -- Project information

project = 'libpqxx'
copyright = '2000-2024, Jeroen T. Vermeulen'
author = 'Jeroen T. Vermeulen'

version = (Path(__file__).parents[2] / 'VERSION').read_text().strip()
release = '.'.join(version.split('.')[:2])

# -- General configuration

extensions = [
    # From readthedocs template:
    'sphinx.ext.duration',
    'sphinx.ext.doctest',
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.intersphinx',

    # Additional:
    'breathe',
    'myst_parser',
]

intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
    'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
}
intersphinx_disabled_domains = ['std']

templates_path = ['_templates']

# -- Options for HTML output

html_theme = 'sphinx_rtd_theme'

# -- Options for EPUB output
epub_show_urls = 'footnote'

pqxx_source = Path(__file__).parents[2]
pqxx_doxygen_xml = pqxx_source / 'docs' / 'source' / 'doxygen-xml'

# XXX: Or are we in a separate build directory?
breathe_projects = {'libpqxx': pqxx_doxygen_xml}
breathe_default_project = 'libpqxx'


if getenv('READTHEDOCS', '').strip() == 'True':
    pqxx_doxygen_xml.mkdir(parents=True, exist_ok=True)
    # XXX: Generate Doxyfile
    # XXX: Run Doxygen
