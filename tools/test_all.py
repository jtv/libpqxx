#! /usr/bin/env python3
"""Brute-force test script: test libpqxx against many compilers etc."""

# Without this, pocketlint does not understand the print function.
from __future__ import print_function

from argparse import ArgumentParser
import os.path
from shutil import rmtree
from subprocess import (
    CalledProcessError,
    check_call,
    DEVNULL,
    )
from sys import (
    stderr,
    stdout,
    )
from tempfile import mkdtemp
from textwrap import dedent


CXX = (
    'g++-7',
    'g++-8',
    'g++-9',
    'clang++-6.0',
    'clang++-7',
    'clang++-8',
    'clang++-9',
    'clang++-10',
    )

STDLIB = (
    '',
    '-stdlib=libc++',
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


class Fail(Exception):
    """A known, well-handled exception.  Doesn't need a traceback."""


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


def make_work_dir():
    """Set up a scratch directory where we can test for compiler support."""
    work_dir = mkdtemp()
    try:
        with open(os.path.join(work_dir, 'check.cxx'), 'w') as source:
            source.write(dedent("""\
                #include <iostream>
                int main()
                {
                    std::cout << "Hello world." << std::endl;
                }
                """))

        return work_dir
    except Exception:
        rmtree(work_dir)
        raise


def check_compiler(work_dir, cxx, stdlib, verbose=False):
    """Is the given compiler combo available?"""
    err_file = os.path.join(work_dir, 'stderr.log')
    if verbose:
        err_output = open(err_file, 'w')
    else:
        err_output = DEVNULL
    try:
        command = [cxx, 'check.cxx']
        if stdlib != '':
            command.append(stdlib)
        check_call(command, cwd=work_dir, stderr=err_output)
    except (OSError, CalledProcessError):
        if verbose:
            with open(err_file) as errors:
                stdout.write(errors.read())
        print("Can't build with '%s %s'.  Skipping." % (cxx, stdlib))
        return False
    else:
        return True


def check_compilers(compilers, stdlibs, verbose=False):
    work_dir = make_work_dir()
    return [
        (cxx, stdlib)
        for stdlib in stdlibs
        for cxx in compilers
        if check_compiler(work_dir, cxx, stdlib, verbose=verbose)
    ]


def try_build(logs_dir, cxx, opt, stdlib, link, link_opts, debug, debug_opts):
    log = os.path.join(
         logs_dir, 'build-%s.out' % '_'.join([cxx, opt, stdlib, link, debug]))
    print("%s... " % log, end='', flush=True)
    configure = [
        "./configure",
        "CXX=%s" % cxx,
        ]

    if stdlib == '':
        configure += [
            "CXXFLAGS=%s" % opt,
            ]
    else:
        configure += [
            "CXXFLAGS=%s %s" % (opt, stdlib),
            "LDFLAGS=%s" % stdlib,
            ]

    configure += [
        "--disable-documentation",
        ] + link_opts + debug_opts

    with open(log, 'w') as output:
        build(configure, output)


def parse_args():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--verbose', '-v', action='store_true')
    parser.add_argument(
        '--compilers', '-c', default=','.join(CXX),
        help="Compilers to use, separated by commas.  Default is %(default)s.")
    parser.add_argument(
        '--optimize', '-O', default=','.join(OPT),
        help=(
            "Alternative optimisation options, separated by commas.  "
            "Default is %(default)s."))
    parser.add_argument(
        '--stdlibs', '-L', default=','.join(STDLIB),
        help=(
            "Comma-separated options for choosing standard library.  "
            "Defaults to %(default)s."))
    parser.add_argument(
        '--logs', '-l', default='.', metavar="DIRECTORY",
        help="Write build logs to DIRECTORY.")
    return parser.parse_args()


def main(args):
    if not os.path.isdir(args.logs):
        raise Fail("Logs location '%s' is not a directory." % args.logs)
    compilers = check_compilers(
        args.compilers.split(','), args.stdlibs.split(','),
        verbose=args.verbose)
    for opt in sorted(args.optimize.split(',')):
        for link, link_opts in sorted(LINK.items()):
            for debug, debug_opts in sorted(DEBUG.items()):
                for cxx, stdlib in compilers:
                    try_build(
                        logs_dir=args.logs, cxx=cxx, opt=opt, stdlib=stdlib,
                        link=link, link_opts=link_opts, debug=debug,
                        debug_opts=debug_opts)


if __name__ == '__main__':
    try:
        main(parse_args())
    except Fail as error:
        stderr.write("%s\n" % error)
