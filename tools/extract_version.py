#! /usr/bin/env python3

"""Print libpqxx version information based on the source tree's VERSION file.

Version strings look like: "<major>.<minor>.<patch>".
"""

from argparse import ArgumentParser, Namespace
from os import getenv
from pathlib import Path
import sys


class Fail(Exception):
    """A well-handled exception.  Show message only, no traceback."""


def parse_args() -> Namespace:
    """Parse command-line arguments."""
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        "--abi",
        "-a",
        action="store_true",
        help="Show ABI version only; omit patch.",
    )
    parser.add_argument(
        "--full",
        "-f",
        action="store_true",
        help="Show full version string (the default).",
    )
    parser.add_argument(
        "--major", "-M", action="store_true", help="Show major version."
    )
    parser.add_argument(
        "--minor", "-m", action="store_true", help="Show minor version."
    )
    parser.add_argument(
        "--patch", "-p", action="store_true", help="Show patch version."
    )
    parser.add_argument(
        "--patch-num",
        "-P",
        action="store_true",
        help="Show patch version if it's a number, or -1 otherwise.",
    )
    parser.add_argument(
        "--srcdir",
        "-s",
        type=Path,
        help="Source directory; defaults to '$srcdir' or '.'.",
    )
    args = parser.parse_args()
    actions = list(
        filter(
            None,
            [
                args.abi,
                args.full,
                args.major,
                args.minor,
                args.patch,
                args.patch_num,
            ],
        )
    )
    if len(actions) > 1:
        raise Fail(
            "Parse at most one of --abi, --full, --major, --minor, --patch, or --patch-num."
        )
    return args


def find_srcdir(args_srcdir: Path) -> Path:
    """Locate the root source directory."""
    return Path(args_srcdir or getenv("srcdir") or Path.cwd())


def read_version(srcdir: Path) -> str:
    """Read the VERSION file."""
    try:
        version = (srcdir / "VERSION").read_text().strip()
    except FileNotFoundError:
        raise Fail(f"Did not find VERSION file in {srcdir}.")
    if len(list(version.split("."))) != 3:
        raise Fail(f"Expected 3 components in version, found '{version}'.")
    return version


def patch_num(patch_str: str) -> int:
    """Return patch number as `int`, or -1 if it's not numeric."""
    if patch_str.isdigit():
        return int(patch_str)
    else:
        return -1


def work() -> None:
    """Do the actual work."""
    args = parse_args()
    version = read_version(find_srcdir(args.srcdir))
    if args.major:
        print(version.split(".")[0])
    elif args.minor:
        print(version.split(".")[1])
    elif args.patch:
        print(version.split(".")[2])
    elif args.patch_num:
        print(patch_num(version.split(".")[2]))
    elif args.abi:
        print(version.rsplit(".", 1)[0])
    else:
        # Default to full version.
        print(version)


def main() -> int:
    """Main entry point."""
    try:
        work()
    except Fail as e:
        print(e, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
