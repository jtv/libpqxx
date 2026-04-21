#! /usr/bin/env python3

"""Extract relevant configuration macros from generated header.

We run `configure` or `cmake` to configure a build.  It spits out a config
header, but there are a lot of definitions in there that we don't actually
want.  Not just because they're not needed, but also because they pollute the
macro namespace.  It'd be fine if it was just inside libpqxx source files, but
the header affects client applications as well.

So, we write our own header file that contains only the macro definitions
with "PQXX" in the name.  Also, we add some boilerplate.
"""

from os import rename
from pathlib import Path
import sys


# TODO: Use argparse.
# TODO: Supporting stdin/stdout might be nice.
def main() -> int:
    """Main entry point.  Returns exit code."""
    infile = Path(sys.argv[1])
    outfile = Path(sys.argv[2])
    tmp = outfile.with_suffix(".tmp")
    with infile.open("r") as instream, tmp.open("w") as outstream:
        for line in instream:
            if "PQXX" in line:
                print(line.strip(), file=outstream)

    # Now move the output file into place.  Doing this afterwards makes it
    # possible to use the same file for input and output.
    # TODO: With Python 3.14 or better, use tmp.rename(outfile).
    rename(tmp, outfile)
    return 0


if __name__ == "__main__":
    sys.exit(main())
