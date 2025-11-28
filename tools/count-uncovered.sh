#! /bin/bash
#
# Show counts of untested source lines based on .gcov files.

set -Cue -o pipefail

if [ -z "${1:-}" ]
then
    cat <<EOF >&2
Show counts of untested source files, based on .gcov files.

Usage: $1 <gcov file or directory>...

You'll probably want to pipe the output through "sort -rn" to get a ranking,
and perhaps the output of that through "head".
EOF

    exit 1
fi

grep -rc '#####' -- "$*" | sed -e 's/^\(.*\):\([0-9]*\)$/\2\t\1/'
