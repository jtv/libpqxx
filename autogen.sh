#! /bin/bash
# Run this to generate the configure script etc.

set -Cue -o pipefail

PQXXVERSION=$(./tools/extract_version.sh)
PQXX_ABI=$(./tools/extract_version.sh --abi)
PQXX_MAJOR=$(./tools/extract_version.sh --major)
PQXX_MINOR=$(./tools/extract_version.sh --minor)
echo "libpqxx version $PQXXVERSION"
echo "libpqxx ABI version $PQXX_ABI"

# Substitute version information into special placeholders.
# This is fully custom stuff and I'd love to get rid of it.
substitute() {
    # (If this were bash, we'd make these locals.)
    infile="$1"
    outfile="$2"

    sed -e "s/@PQXXVERSION@/$PQXXVERSION/g" \
        -e "s/@PQXX_MAJOR@/$PQXX_MAJOR/g" \
        -e "s/@PQXX_MINOR@/$PQXX_MINOR/g" \
        -e "s/@PQXX_ABI@/$PQXX_ABI/g" \
        "$infile" >|"$outfile.tmp"

    mv "$outfile.tmp" "$outfile"
}


# Use templating system to generate various Makefiles.
expand_templates() {
    for template in "$@"
    do
        ./tools/template2mak.py "$template" "${template%.template}"
    done
}


# We have two kinds of templates.  One uses our custom templating tool.  And
# a few others simply have some substitutions done.
readarray -t < <(find . -name \*.template) templates
expand_templates "${templates[@]}"
substitute include/pqxx/version.hxx.template include/pqxx/version.hxx
substitute include/pqxx/doc/mainpage.md.template include/pqxx/doc/mainpage.md

# Generate feature test snippets for C++ features that we simply detect by
# checking a C++ feature test macro.
./tools/generate_cxx_checks.py

# Generate autoconf and CMake configuration for our feature test snippets.
./tools/generate_check_config.py

autoheader
libtoolize --force --automake --copy
aclocal -I . -I config/m4
automake --add-missing --copy
autoconf

echo "Done."
