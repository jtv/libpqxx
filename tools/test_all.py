#! /usr/bin/env python3
"""Brute-force test script: test libpqxx against many compilers etc.

This script makes no changes in the source tree; all builds happen in
temporary directories.

To make this possible, you may need to run "make distclean" in the
source tree.  The configure script will refuse to configure otherwise.
"""

# Without this, pocketlint does not yet understand the print function.
from __future__ import print_function

from argparse import ArgumentParser
from contextlib import contextmanager
from os import (
    cpu_count,
    getcwd,
    )
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


CPUS = cpu_count()

GCC_VERSIONS = list(range(7, 12))
GCC = ['g++-%d' % ver for ver in GCC_VERSIONS]
CLANG_VERSIONS = list(range(7, 12))
CLANG = ['clang++-6.0'] + ['clang++-%d' % ver for ver in CLANG_VERSIONS]
CXX = GCC + CLANG

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


# CMake "generators."  Maps a value for cmake's -G option (None for default) to
# a command line to run.
#
# I prefer Ninja if available, because it's fast.  But hey, the default will
# work.
#
# Maps the name of the generator (as used with cmake's -G option) to the
# actual command line needed to do the build.
CMAKE_GENERATORS = {
    'Ninja': ['ninja'],
    None: ['make', '-j%d' % CPUS],
}


class Fail(Exception):
    """A known, well-handled exception.  Doesn't need a traceback."""


def run(cmd, output, cwd=None):
    """Run a command, write output to file-like object."""
    command_line = ' '.join(cmd)
    output.write("%s\n\n" % command_line)
    check_call(cmd, stdout=output, stderr=output, cwd=cwd)


def report(output, message):
    """Report a message to output, and standard output."""
    print(message)
    output.write('\n\n')
    output.write(message)
    output.write('\n')


def report_success(output):
    """Report a succeeded build."""
    report(output, "OK")
    return True


def report_failure(output, error):
    """Report a failed build."""
    report(output, "FAIL: %s" % error)
    return False


def file_contains(path, text):
    """Does the file at path contain text?"""
    with open(path) as stream:
        for line in stream:
            if text in line:
                return True
    return False


def build(configure, output):
    """Perform a full configure-based build."""
    with tmp_dir() as work_dir:
        try:
            run(configure, output, cwd=work_dir)
        except CalledProcessError:
            output.flush()
            if file_contains(output.name, "make distclean"):
                # Looks like that special "configure" error where the source
                # tree is already configured.  Tell the user about this special
                # case without requiring them to dig deeper.
                raise Fail(
                    "Configure failed.  "
                    "Did you remember to 'make distclean' the source tree?")
            return report_failure("configure failed.")

        try:
            run(['make', '-j%d' % CPUS], output, cwd=work_dir)
            run(['make', '-j%d' % CPUS, 'check'], output, cwd=work_dir)
        except CalledProcessError as err:
            return report_failure(output, err)
        else:
            return report_success(output)


@contextmanager
def tmp_dir():
    """Create a temporary directory, and clean it up again."""
    tmp = mkdtemp()
    try:
        yield tmp
    finally:
        rmtree(tmp)


def write_check_code(work_dir):
    """Write a simple C++ program so we can tesst whether we can compile it.

    Returns the file's full path.
    """
    path = os.path.join(work_dir, "check.cxx")
    with open(path, 'w') as source:
        source.write(dedent("""\
            #include <iostream>
            int main()
            {
                std::cout << "Hello world." << std::endl;
            }
            """))

    return path


def check_compiler(work_dir, cxx, stdlib, check, verbose=False):
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
    """Check which compiler configurations are viable."""
    with tmp_dir() as work_dir:
        check = write_check_code(work_dir)
        return [
            (cxx, stdlib)
            for stdlib in stdlibs
            for cxx in compilers
            if check_compiler(
                work_dir, cxx, stdlib, check=check, verbose=verbose)
        ]


def try_build(
        logs_dir, cxx, opt, stdlib, link, link_opts, debug, debug_opts
    ):
    """Attempt to build in a given configuration."""
    log = os.path.join(
        logs_dir, 'build-%s.out' % '_'.join([cxx, opt, stdlib, link, debug]))
    print("%s... " % log, end='', flush=True)
    configure = [
        os.path.join(getcwd(), "configure"),
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


def prepare_cmake(work_dir, output, verbose=False):
    """Set up a CMake build dir, ready to build.

    Returns the directory, and if successful, the command you need to run in
    order to do the build.
    """
    if verbose:
        print("\nLooking for CMake generator.")
    source_dir = getcwd()
    for gen, cmd in CMAKE_GENERATORS.items():
        name = gen or '<default>'
        cmake = ['cmake', source_dir]
        if gen is not None:
            cmake += ['-G', gen]
        if verbose:
            print("Trying CMake with %s." % name)

        try:
            run(cmake, output=output, cwd=work_dir)
        except FileNotFoundError:
            print("No cmake found.  Skipping.")
            return None
        except CalledProcessError:
            print("CMake generator %s is not available.  Skipping." % name)
        else:
            return cmd

    return None


def build_with_cmake(logs_dir, verbose=False):
    """Build using CMake.  Use the first generator that works."""
    log = os.path.join(logs_dir, "build-cmake.out")
    print("%s... " % log, end='', flush=True)
    with tmp_dir() as work_dir, open(log, 'w') as output:
        try:
            command = prepare_cmake(work_dir, output, verbose)
            if command is None:
                return report_fail(
                    output,
                    "No CMake generators found.  Skipping CMake build.")
            else:
                if verbose:
                    print("Building with CMake and '%s'." % ' '.join(command))
                run(command, output, cwd=work_dir)
                run(['test/unit/unit_runner'], output, cwd=work_dir)
                run(['test/runner'], output, cwd=work_dir)
        except CalledProcessError:
            return report_fail(output, "CMake build failed.")
        else:
            return report_success(output)


def parse_args():
    """Parse command-line arguments."""
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--verbose', '-v', action='store_true')
    parser.add_argument(
        '--compilers', '-c', default=','.join(CXX),
        help="Compilers, separated by commas.  Default is %(default)s.")
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
        '--logs', '-l', default='.', metavar='DIRECTORY',
        help="Write build logs to DIRECTORY.")
    parser.add_argument(
        '--jobs', '-j', default=CPUS, metavar='CPUS',
        help=(
            "When running 'make', run up to CPUS concurrent processes.  "
            "Defaults to %(default)s."))
    parser.add_argument(
        '--minimal', '-m', action='store_true',
        help="Make it as short a run as possible.  For testing this script.")
    return parser.parse_args()


def main(args):
    """Do it all."""
    if not os.path.isdir(args.logs):
        raise Fail("Logs location '%s' is not a directory." % args.logs)

    if args.verbose:
        print("\nChecking available compilers.")

    compiler_candidates = args.compilers.split(',')
    compilers = check_compilers(
        compiler_candidates, args.stdlibs.split(','),
        verbose=args.verbose)
    if list(compilers) == []:
        raise Fail(
            "Did not find any viable compilers.  Tried: %s."
            % ', '.join(compiler_candidates))

    opt_levels = args.optimize.split(',')
    link_types = LINK.items()
    debug_mixes = DEBUG.items()

    if args.minimal:
        compilers = compilers[:1]
        opt_levels = opt_levels[:1]
        link_types = list(link_types)[:1]
        debug_mixes = list(debug_mixes)[:1]

    print("\nStarting builds.")
    for opt in sorted(opt_levels):
        for link, link_opts in sorted(link_types):
            for debug, debug_opts in sorted(debug_mixes):
                for cxx, stdlib in compilers:
                    try_build(
                        logs_dir=args.logs, cxx=cxx, opt=opt, stdlib=stdlib,
                        link=link, link_opts=link_opts, debug=debug,
                        debug_opts=debug_opts)

    if args.verbose:
        print("\nBuilding with CMake.")
    build_with_cmake(args.logs, verbose=args.verbose)


if __name__ == '__main__':
    try:
        main(parse_args())
    except Fail as error:
        stderr.write("%s\n" % error)
