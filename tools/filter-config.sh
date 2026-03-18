#! /bin/bash
#
# We run configure or cmake to configure a build.  It spits out a config
# header, but there are a lot of definitions in there that we don't actually
# want.  Not just because they're not needed, but also because they pollute the
# macro namespace.
#
# So, we write our own header file that contains only the macro definitions
# starting with "PQXX".  Also, we add some boilerplate.
#
# Usage: filter-config.sh <input.h> <output.h>

set -Cue -o pipefail

INPUT="$1"
OUTPUT="$2"

cat <<EOF >|"$OUTPUT"
#ifndef PQXX_CONFIG_COMPILER_H
#define PQXX_CONFIG_COMPILER_H

EOF

grep PQXX -- "$INPUT" >>"$OUTPUT"

cat <<EOF >>"$OUTPUT"

#endif
EOF
