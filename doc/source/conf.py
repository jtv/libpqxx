# Configuration file for the Sphinx documentation builder.

from pathlib import Path


project = 'libpqxx'
copyright = '2000-2024, Jeroen T. Vermeulen'
author = 'Jeroen T. Vermeulen'

version = (Path(__file__).parents[2] / 'VERSION').read_text().strip()
release = '.'.join(version.split('.')[:2])

extensions = [
    # TODO: Not actually using Sphinx for now.  Should we?
    # From readthedocs template:
    # 'sphinx.ext.duration',
    # 'sphinx.ext.doctest',
    # 'sphinx.ext.autodoc',
    # 'sphinx.ext.autosummary',
    # 'sphinx.ext.intersphinx',

    # Additional:
    # TODO: Not using Breathe for now.  Should we?
    # 'breathe',
    'myst_parser',
]

# TODO: What do these actually mean?
# intersphinx_mapping = {
#     'python': ('https://docs.python.org/3/', None),
#     'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
# }
# intersphinx_disabled_domains = ['std']

templates_path = ['_templates']

# -- Options for HTML output

html_theme = 'sphinx_rtd_theme'
