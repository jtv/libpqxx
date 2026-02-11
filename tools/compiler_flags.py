#! /usr/bin/env python3

"""Check whether the compiler accepts various options.

Prints any of the flags that are accepted when used in combination with the
base command line and the previously accepted flags.
"""

from argparse import (
    ArgumentParser,
    FileType,
    Namespace,
)
from pathlib import Path
from subprocess import (
    DEVNULL,
    run,
)
from tempfile import NamedTemporaryFile


EPILOG = """The flags file may contain comments, on lines starting with '#'
as the first non-whitespace character.  Any whitespace other than newlines is
ignored, as are blank lines.
"""

def parse_args() -> Namespace:
    """Parse command-line options."""
    parser = ArgumentParser(description=__doc__, epilog=EPILOG)

    parser.add_argument(
        "-c",
        "--command",
        help="The basic compiler command line, e.g. 'c++ -std=c++20 -Werror'.",
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


def run_quietly(cmd: str) -> bool:
    """Run `cmd`, suppressing output; return whether it succeeds.

    **This will run `cmd` through shell interpretation.**
    """
    return run(cmd, stdout=DEVNULL, stderr=DEVNULL, shell=True).returncode == 0


def compiler_accepts(
    command: str, source: Path, flag: str, prev: str = ""
) -> bool:
    """Return whether the compiler seems to accept `flag`."""
    return run_quietly(f"{command} -c {source} {prev} {flag}")


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
            if compiler_accepts(
                args.command, Path(src), flag, prev=" ".join(good_flags)
            ):
                good_flags.append(flag)

    sep = " " if args.single_line else "\n"
    print(sep.join(good_flags), file=args.output)


if __name__ == "__main__":
    main()
