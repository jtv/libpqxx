# Configuration file for the Sphinx documentation builder.

from os import getenv
from pathlib import Path
from subprocess import run


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

pqxx_source = Path(__file__).parents[2]
pqxx_docs = pqxx_source / 'docs'
pqxx_html = pqxx_docs / 'html'

# TODO: Not really using Sphinx/Breathe for now.  Should we?
# pqxx_doxygen_xml = pqxx_source / 'docs' / 'doxygen-xml'
# breathe_projects = {'libpqxx': pqxx_doxygen_xml}
# breathe_default_project = 'libpqxx'


if getenv('READTHEDOCS', '') == 'True':
    # We're not actually using Sphinx.  We're just using this Sphinx config
    # file as a launcher for our own build.  There is a feature in beta on
    # readthedocs.org to let us just write our own build procedure:
    #
    # https://docs.readthedocs.io/en/stable/build-customization.html
    #
    # (Look for "override the build process")..

    # TODO: Not really using Sphinx/Breathe for now.  Should we?
    # pqxx_doxygen_xml.mkdir(parents=True, exist_ok=True)

    pqxx_html.mkdir(parents=True)

    # Configure, so we have config & version headers.
    run(
        [
            './configure',
            '--disable-shared',
            'CXXFLAGS=-O0 -std=c++23',
        ],
        cwd=pqxx_source,
        check=True,
        timeout=100,
    )

    # Now run Doxygen.  Not clear to me how else we're supposed to do this,
    # but Sphinx apparently assumes that we've already done this!
    run(
        ['doxygen'],
        cwd=pqxx_docs,
        check=True,
        timeout=300,
    )

    pqxx_output_dir = getenv('READTHEDOCS_OUTPUT')
    if pqxx_output_dir:
        # Move the HTML output to where readthedocs expects it.
	pqxx_output_dir = Path(pqxx_output_dir)
	pqxx_output_dir.mkdir(parents=True)
        pqxx_html.rename(pqxx_rtd_html / 'html')
