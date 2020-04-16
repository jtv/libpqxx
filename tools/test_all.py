#! /usr/bin/env python3
"""Brute-force test script: test libpqxx against many compilers etc.

This script makes no changes in the source tree; all builds happen in
temporary directories.

To make this possible, you may need to run "make distclean" in the
source tree.  The configure script will refuse to configure otherwise.
"""

# Without this, pocketlint does not yet understand the print function.
from __future__ import print_function

from abc import (
    ABCMeta,
    abstractmethod,
    )
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


class Skip(Exception):
    """"We're not doing this build.  It's not an error though."""


def run(cmd, output, cwd=None):
    """Run a command, write output to file-like object."""
    command_line = ' '.join(cmd)
    output.write("%s\n\n" % command_line)
    check_call(cmd, stdout=output, stderr=output, cwd=cwd)


def report(output, message):
    """Report a message to output, and standard output."""
    print(message, flush=True)
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
        command = [cxx, check]
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


class Config:
    """Configuration for a build."""
    __metaclass__ = ABCMeta

    @abstractmethod
    def name(self):
        """Return an identifier for this build configuration."""

    def make_log_name(self):
        """Compose log file name for this build."""
        return "build-%s.out" % self.name()


class Build:
    """A pending or ondoing build, in its own directory.

    Each step returns True for Success, or False for failure.
    """
    def __init__(self, logs_dir, config=None):
        self.config = config
        self.log = open(os.path.join(logs_dir, config.make_log_name()), 'w')
        self.work_dir_context = tmp_dir()
        self.work_dir = self.work_dir_context.__enter__()

    def report(self, stage):
        """Print a message saying what we're about to do."""
        print(
            "%s - %s... " % (self.config.make_log_name(), stage),
            end='', flush=True)

    def clean_up(self):
        """Delete the build tree."""
        self.work_dir_context.__exit__(None, None, None)
        self.log.close()

    @abstractmethod
    def configure(self):
        """Prepare for a build.

        This step is basically single-threaded.
        """

    @abstractmethod
    def build(self):
        """Build the code, including the tests.  Don't run tests though."""


    def test(self):
        """Run tests."""
        try:
            run(
                [os.path.join(os.path.curdir, 'test', 'runner')], self.log,
                cwd=self.work_dir)
        except CalledProcessError:
            return report_failure(self.log, "Tests failed.")
        else:
            return report_success(self.log)


class AutotoolsConfig(Config):
    """A combination of build options for the "configure" script."""
    def __init__(self, cxx, opt, stdlib, link, link_opts, debug, debug_opts):
        self.cxx = cxx
        self.opt = opt
        self.stdlib = stdlib
        self.link = link
        self.link_opts = link_opts
        self.debug = debug
        self.debug_opts = debug_opts

    def name(self):
        return '_'.join([
            self.cxx, self.opt, self.stdlib, self.link, self.debug])


class AutotoolsBuild(Build):
    """Build using the "configure" script."""
    __metaclass__ = ABCMeta

    def configure(self):
        configure = [
            os.path.join(getcwd(), "configure"),
            "CXX=%s" % self.config.cxx,
            ]

        if self.config.stdlib == '':
            configure += [
                "CXXFLAGS=%s" % self.config.opt,
            ]
        else:
            configure += [
                "CXXFLAGS=%s %s" % (self.config.opt, self.config.stdlib),
                "LDFLAGS=%s" % self.config.stdlib,
                ]

        configure += [
            "--disable-documentation",
            ] + self.config.link_opts + self.config.debug_opts

        try:
            run(configure, self.log, cwd=self.work_dir)
        except CalledProcessError:
            self.log.flush()
            if file_contains(self.log.name, "make distclean"):
                # Looks like that special "configure" error where the source
                # tree is already configured.  Tell the user about this
                # special case without requiring them to dig deeper.
                raise Fail(
                    "Configure failed.  "
                    "Did you remember to 'make distclean' the source tree?")
            return report_failure(self.log, "configure failed.")
        else:
            return report_success(self.log)

    def build(self):
        try:
            run(['make', '-j%d' % CPUS], self.log, cwd=self.work_dir)
            # Passing "TESTS=" like this will suppress the actual running of
            # the tests.  We do that in the nest stage.
            run(
                ['make', '-j%d' % CPUS, 'check', 'TESTS='],
                self.log, cwd=self.work_dir)
        except CalledProcessError as err:
            return report_failure(self.log, err)
        else:
            return report_success(self.log)


class CMakeConfig(Config):
    """Configuration for a CMake build."""
    def __init__(self):
        # The CMakeBuild figures this out in the configure stage.
        self.command = None

    def name(self):
        return "cmake"


class CMakeBuild(Build):
    """Build using CMake.

    Ignores the config for now.
    """
    __metaclass__ = ABCMeta

    def configure(self):
        source_dir = getcwd()
        for gen, cmd in CMAKE_GENERATORS.items():
            name = gen or '<default>'
            cmake = ['cmake', source_dir]
            if gen is not None:
                cmake += ['-G', gen]

            try:
                run(cmake, output=self.log, cwd=self.work_dir)
            except FileNotFoundError:
                raise Skip("No cmake found.")
            except CalledProcessError:
                print(
                    "CMake generator %s is not available.  Skipping." % name)
            else:
                self.config.command = cmd
                return report_success(self.log)

        raise Skip("Did not find any working CMake generators.")

    def build(self):
        print("%s... " % self.log.name, end='', flush=True)
        try:
            run(self.config.command, self.log, cwd=self.work_dir)
            run(['test/runner'], self.log, cwd=self.work_dir)
            return report_success(self.log)
        except CalledProcessError:
            return report_failure(self.log, "CMake build failed.")
        else:
            return report_success(self.log)


def run_step(builds, step_name, step):
    """Run `build(step) for each build in builds.  Return passing builds."""
    succeeded = []
    for build in builds:
        try:
            build.report(step_name)
            if step(build):
                succeeded.append(build)
        except Skip as error:
            print("Skipping: %s" % error)
    return succeeded


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

    if args.verbose:
        print("\nStarting builds.")

    builds = [
        AutotoolsBuild(
            args.logs,
            AutotoolsConfig(
                opt=opt, link=link, link_opts=link_opts, debug=debug,
                debug_opts=debug_opts, cxx=cxx, stdlib=stdlib))
        for opt in sorted(opt_levels)
        for link, link_opts in sorted(link_types)
        for debug, debug_opts in sorted(debug_mixes)
        for cxx, stdlib in compilers
    ] + [
        CMakeBuild(args.logs, CMakeConfig())
    ]

    try:
        to_build = run_step(
            builds, "configure", lambda build: build.configure())
        to_test = run_step(
            to_build, "build", lambda build: build.build(),
            verbose=args.verbose)
        done = run_step(
            to_test, "test", lambda build: build.test(),
            verbose=args.verbose)
    finally:
        for build in builds:
            build.clean_up()

    print("Passed %d out of %d builds." % (len(done), len(builds)))


if __name__ == '__main__':
    try:
        main(parse_args())
    except Fail as failure:
        stderr.write("%s\n" % failure)
