#! /usr/bin/env python3

"""Extract relevant configuration macros from generated header.

We run `configure` or `cmake` to configure a build.  It spits out a config
header, but there are a lot of definitions in there that we don't actually
want.  Not just because they're not needed, but also because they pollute the
macro namespace.  It'd be fine if it was just inside libpqxx source files, but
the header affects client applications as well.

So, we rewrite the header (or its template) to contain only the macro
definitions with "PQXX" in the name.
"""

from argparse import (
    ArgumentParser,
    Namespace,
)
from pathlib import Path
import sys


# We prefix this to the header's text.  It suppresses a clang-tidy warning
# about there not being an ifdef/define pair to prevent multiple inclusion.
HEADER = "// NOLINT(llvm-header-guard)"


def parse_args() -> Namespace:
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("infile", type=str, help="Input header.")
    parser.add_argument(
        "--outfile",
        "-o",
        type=str,
        help="Optional output header; defaults to same as input.",
    )
    return parser.parse_args()


def main() -> int:
    """Main entry point.  Returns exit code."""
    args = parse_args()
    infile = Path(args.infile)
    outfile = Path(args.outfile or args.infile)
    tmp = outfile.with_suffix(".tmp")
    with infile.open("r") as instream, tmp.open("w") as outstream:
        print(HEADER, file=outstream)
        for line in instream:
            if "PQXX" in line:
                print(line.strip(), file=outstream)

    # Now move the output file into place.  Doing this afterwards makes it
    # possible to use the same file for input and output.
    tmp.replace(outfile)
    return 0


if __name__ == "__main__":
    sys.exit(main())
