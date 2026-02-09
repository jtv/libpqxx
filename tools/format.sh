#! /bin/bash
#
# Reformat source code using clang-format.
#
# This script requires "uv" to be installed.

set -Cue -o pipefail

# Reformat C++ files.
find . -name '*.[ch]xx' -print0 | xargs -0 clang-format -i


# Reformat CMake files.  (Requires "uv" to be installed.)
find . -name '*.cmake' -exec \
    uv run --with='pyaml,cmake-format' cmake-format -i '{}' '+'

if which black >/dev/null
then
    black -l80 .
else
    uv run --with=black==24.2.0 black -l80 .
fi
