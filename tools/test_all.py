#! /usr/bin/env python3
"""Brute-force test script: test libpqxx against many compilers etc."""

from __future__ import print_function

from argparse import ArgumentParser
from subprocess import (
    CalledProcessError,
    check_call,
    )

CXX = (
    'g++-7',
    'g++-8',
    'g++-9',
    'clang++-6.0',
    'clang++-7',
    'clang++-9',
    )

OPT = ('-O0', '-O3')

LINK = {
    'static': ['--enable-static', '--disable-dynamic'],
    'dynamic': ['--disable-static', '--enable-dynamic'],
}

DEBUG = {
    'plain': [],
    'audit': ['--enable-audit'],
    'maintainer': ['--enable-maintainer-mode'],
    'full': ['--enable-audit', '--enable-maintainer-mode'],
}


def run(cmd, output):
    command_line = ' '.join(cmd)
    output.write("%s\n\n" % command_line)
    check_call(cmd, stdout=output, stderr=output)


def build(configure, output):
    try:
        run(configure, output)
        run(['make', '-j8', 'clean'], output)
        run(['make', '-j8'], output)
        run(['make', '-j8', 'check'], output)
    except CalledProcessError as err:
        print("FAIL: %s" % err)
        output.write("\n\nFAIL: %s\n" % err)
    else:
        print("OK")
        output.write("\n\nOK\n")


def parse_args():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        '--compilers', '-c', default=','.join(CXX),
        help="Compilers to use, separated by commas.  Default is %(default)s.")
    parser.add_argument(
        '--optimize', '-O', default=','.join(OPT),
        help=(
            "Alternative optimisation options, separated by commas.  "
            "Default is %(default)s."))
    return parser.parse_args()


def main(args):
    for cxx in sorted(args.compilers.split(',')):
        for opt in sorted(args.optimize.split(',')):
            for link, link_opts in sorted(LINK.items()):
                for debug, debug_opts in sorted(DEBUG.items()):
                    log = 'build-%s.out' % '_'.join(
                        [cxx, opt, link, debug])
                    print("%s... " % log, end='', flush=True)
                    configure = [
                        './configure',
                        'CXX=%s' % cxx,
                        'CXXFLAGS=%s' % opt,
                        '--disable-documentation',
                        ] + link_opts + debug_opts
                    with open(log, 'w') as output:
                        build(configure, output)

if __name__ == '__main__':
    main(parse_args())
