#! /usr/bin/env python3

"""Extract relevant configuration macros from generated header.

We rnu `configure` or `cmake` to configure a build.  It spits out a config
header, but there are a lot of definitions in there that we don't actually
want.  Not just because they're not needed, but also because they pollute the
macro namespace.  It'd be fine if it was just inside libpqxx source files, but
the header affects client applications as well.

So, we write our own header file that contains only the mcaro definitions
starting with "PQXX".  Also, we add some boilerplate.
"""

from pathlib import Path
import sys

HEADER = """\
#ifndef PQXX_CONFIG_COMPILER_H
#define PQXX_CONFIG_COMPILER_H
"""

FOOTER = """\
#endif
"""


def main() -> int:
    """Main entry point.  Returns exit code."""
    infile = Path(sys.argv[1])
    outfile = Path(sys.argv[2])
    with infile.open("r") as instream, outfile.open("w") as outstream:
        print(HEADER, file=outstream)
        for line in instream:
            if "PQXX" in line:
                print(line.strip(), file=outstream)
        print(FOOTER, file=outstream, end="")

    return 0


if __name__ == "__main__":
    sys.exit(main())
