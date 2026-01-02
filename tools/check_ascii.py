#! /usr/bin/env python3

"""Check that all source files are pure ASCII."""

from argparse import ArgumentParser
from pathlib import Path
import sys


def find_source_files(srcdir: Path) -> list[Path]:
    """Find source file paths in `srcdir`."""
    paths = []
    for (dirpath, _, filenames) in srcdir.walk():
        if dirpath.relative_to(srcdir) == Path('include') / 'pqxx':
            # The header code in this directory is in files without suffixes.
            for filename in filenames:
                path = dirpath / filename
                if path.suffix in ('.cxx', '.hxx') or '.' not in filename:
                    paths.append(path)
        else:
            for filename in filenames:
                path = dirpath / filename
                if path.suffix in ('.cxx', '.hxx'):
                    paths.append(path)
    return paths


def check_file(path: Path) -> list[int]:
    """Return any line numbers in file at `path` that are not pure ASCII.

    Actually we're not properly checking _lines,_ because there is no such
    notion in a binary file.  But we'll pretend we can split the contents by
    '\n', since it's mostly supposed to be ASCII (where this works).
    """
    lines = path.read_bytes().split(b'\n')
    bad_lines = []
    for n, data in enumerate(lines, start=1):
        # Why in blazes am I getting an int at the end of the file!?
        if isinstance(data, bytes):
            try:
                data.decode('ascii')
            except UnicodeDecodeError:
                bad_lines.append(n)
    return bad_lines


def parse_args():
    """Parse command line."""
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        '-v', '--verbose', action='append_const', default=[],
        help="Print more detail.  Double this for even more.")
    parser.add_argument(
        '-s', '--srcdir', default=Path(), type=Path,
       help="Source directory.  Defaults to current directory.")
    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    files = sorted(find_source_files(args.srcdir))
    if len(args.verbose) > 1:
        print('\n'.join(str(path) for path in files))

    ok = True
    for path in files:
        lines = check_file(path)
        if lines != []:
            ok = False
            print(f"*** Non-ASCII characters in {path}: ***", file=sys.stderr)
            for line in lines:
                print(f" - line {line}", file=sys.stderr)

    if ok:
        return 0
    else:
        return 1


if __name__ == '__main__':
    sys.exit(main())
