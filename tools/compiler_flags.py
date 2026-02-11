#! /usr/bin/env python3

"""Check whether the compiler accepts various options.

Prints any fl
"""

from argparse import (
    ArgumentParser,
    FileType,
    Namespace,
)
from os import getenv
from pathlib import Path
from subprocess import (
    DEVNULL,
    run,
)
from tempfile import NamedTemporaryFile


def parse_args() -> Namespace:
    """Parse command-line options."""
    parser = ArgumentParser(description=__doc__)

    parser.add_argument(
        "-c",
        "--cxx",
        default=getenv("CXX", "c++"),
        help="C++ compiler.  Defaults to $CXX, or 'c++'.",
    )
    parser.add_argument(
        "-f",
        "--flags",
        default="-",
        type=FileType("r"),
        help="File with compiler flags to check, one per line; '-' for stdin.",
    )
    parser.add_argument(
        "-s",
        "--single-line",
        action="store_true",
        default=False,
        help="Print all command-line options on a single line.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="-",
        type=FileType("w"),
        help="Output file, or '-' for stdout (the default).",
    )

    return parser.parse_args()


def run_quietly(cmd: list[str | Path]) -> bool:
    """Run `cmd`, suppressing output; return whether it succeeds."""
    return run(cmd, stdout=DEVNULL, stderr=DEVNULL).returncode == 0


def compiler_accepts(
    cxx: Path, source: Path, flag: str, prev: None | list[str]
) -> bool:
    """Return whether the compiler seems to accept `flag`."""
    if prev is None:
        prev = []
    return run_quietly([cxx, "-c", source] + prev + [flag])


def make_source():
    """Create a minimal source file to compile.  Doesn't have to link.

    Can be used as a cnotext manager.
    """
    source = NamedTemporaryFile()
    # As it happens, an empty file will do.
    source.flush()
    return source


def main() -> None:
    """Main entry point."""
    args = parse_args()
    good_flags = []
    with make_source() as source:
        src = source.name
        for flag in args.flags:
            flag = flag.strip()
            if flag == "" or flag.startswith("#"):
                continue
            if compiler_accepts(args.cxx, Path(src), flag, prev=good_flags):
                good_flags.append(flag)

    sep = " " if args.single_line else "\n"
    print(sep.join(good_flags), file=args.output)


if __name__ == "__main__":
    main()
