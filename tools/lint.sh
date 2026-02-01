#! /bin/bash
#
# Routine sanity checks for libpqxx source tree.
#
# This script requires "uv" to be installed.
#
# Optionally, set environment variable "srcdir" to the source directory.  It
# defaults to the parent directory of the one where this script is.  This trick
# requires bash (or a close equivalent) as the shell.
#
# Set PQXX_LINT=skip to suppress lint checks.

set -Cue -o pipefail

if [ "${PQXX_LINT:-}" = "skip" ]
then
    echo "PQXX_LINT is set to 'skip'.  Skipping lint check."
    exit 0
fi

SRCDIR="${srcdir:-$(dirname "${BASH_SOURCE[0]}")/..}"
PQXXVERSION="$(cd "$SRCDIR" && "$SRCDIR/tools/extract_version.sh")"

ARGS="${1:-}"


# Check that all source code is pure ASCII.
#
# I'd love to have rich Unicode, but I can live without it.  Mainly we don't
# want any surprises in contributions.
#
# MD files are different.  I love me some Unicode in there.  They're UTF-8.
check_ascii() {
    "$SRCDIR/tools/check_ascii.py" -v
}


# This version must be at the top of the NEWS file.
check_news_version() {
    if ! head -n1 "$SRCDIR/NEWS" | grep -q "^$PQXXVERSION\$"
    then
        cat <<EOF >&2
Version $PQXXVERSION is not at the top of NEWS.
EOF
        exit 1
    fi
}


# Count number of times header $1 is included from each of given input files.
# Output is lines of <filename>:<count>, one line per file, sorted.
count_includes() {
    local HEADER_NAME PAT
    HEADER_NAME="$1"
    shift
    PAT='^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"]'"$HEADER_NAME"'[>"]'
    # It's OK for the grep to fail.
    find "$SRCDIR/include/pqxx" -type f -print0 |
        ( xargs -0 grep -c "$PAT" || true ) |
        sort
}


# Check that any includes of $1-pre.hxx are matched by $1-post.hxx ones.
# TODO: Probably time to rewrite this in python.
match_pre_post_headers() {
    local NAME PRE POST
    NAME="$1"
    TEMPDIR="$(mktemp -d)"

    # In case we get interrupted before we can clean up $TEMPDIR ourselves.
    # (Next invocation will overwrite the trap.)
    trap 'rm -rf -- "$TEMPDIR"'  EXIT

    PRE="$TEMPDIR/pre"
    POST="$TEMPDIR/post"
    count_includes "$SRCDIR/$NAME-pre.hxx" >"$PRE"
    count_includes "$SRCDIR/$NAME-post.hxx" >"$POST"
    DIFF="$(diff "$PRE" "$POST")" || true

    rm -rf -- "$TEMPDIR"

    if test -n "$DIFF"
    then
        cat <<EOF >&2
Mismatched pre/post header pairs:

$DIFF
EOF
        exit 1
    fi
}


# Any file that includes header-pre.hxx must also include header-post.hxx, and
# vice versa.  Similar for ignore-deprecated-{pre|post}.hxx.
check_compiler_internal_headers() {
    match_pre_post_headers "pqxx/internal/header"
    match_pre_post_headers "pqxx/internal/ignore-deprecated"
}


check_yaml() {
    yamllint -- \
        "$SRCDIR" "$SRCDIR/.clang-format" "$SRCDIR/.clang-tidy" \
        "$SRCDIR/.cmake-format"
}


cpplint() {
    local dialect includes

    if [ -e compile_commands.json ]
    then
        # This wrapper script runs in parallel, making it run faster on most
        # systems.  It uses the "compilation database" that cmake writes into
        # compile_commands.json.
        #
        # It does look as if there's massive overhead to concurrent runs.  A
        # run with -j16 (on a 16-core system) ran less than twice as fast, but
        # racked up almost ten times as much CPU time.  A run with -j1 took
        # more than four times as long as the original tool.  Around -j5 it
        # overtakes the original tool, but using five times as much CPU.
        # At -j8 it actually seems to run faster than at -j16, and still with
        # "only" five times the original CPU time used.
        run-clang-tidy -q -j8 -warnings-as-errors=*
    elif [ -e compile_flags ]
    then
        # The configure-based build writes its compiler options into this
        # file, so we use that to figure out preprocessor paths, language
        # flag, etc.
        #
        # Pick out relevant flags, but leave out the rest.  If we're not
        # compiling with clang, compile_flags may contain options that
        # clang-tidy doesn't recognise.
        readarray -t dialect < <( \
            grep -o -- '-std=c++[^[:space:]]*' compile_flags || true )
        readarray -t includes < <( \
            grep -o -- '-I[[:space:]]*[^[:space:]]*' compile_flags || true )
        clang-tidy \
            --quiet '-warnings-as-errors=*' \
            "$SRCDIR"/src/*.cxx "$SRCDIR"/tools/*.cxx \
            "$SRCDIR"/test/*.cxx \
            -- \
            -I"$SRCDIR/include" -Iinclude "${dialect[@]}" "${includes[@]}"

    else
        cat <<EOF >&2
Could not find compile flags for clang-tidy run.

Run this script from a build directry prepared with either configure or cmake.
EOF
        exit 1
    fi
}


pylint() {
    if which pyflakes >/dev/null
    then
        pyflakes "$SRCDIR"
    else
        uv -q run --with=pyflakes==3.4.0 pyflakes "$SRCDIR"
    fi

    if which ruff >/dev/null
    then
        ruff check -q "$SRCDIR"
    else
        uv -q run --with=ruff==0.14.14 ruff check -q "$SRCDIR"
    fi

    if which pyrefly >/dev/null
    then
        pyrefly check .
    else
        uv -q run --with=pyrefly==0.50.1 pyrefly check --summary=none "$SRCDIR"
    fi
}


shelllint() {
    local cmd

    if which shellcheck >/dev/null
    then
        cmd=(shellcheck)
    else
        cmd=(uv -q run --with=shellcheck.py shellcheck)
    fi

    find "$SRCDIR/tools" -name '*.sh' -exec "${cmd[@]}" '{}' '+'
}


mdlint() {
    if which mdl >/dev/null
    then
        find "$SRCDIR" -name \*.md -exec \
            mdl -s "$SRCDIR/.mdl_style.rb" -c "$SRCDIR/.markdownlint.yaml" \
            '{}' '+'
    fi
}


check_cmake() {
    find "$SRCDIR" \( -name '*.cmake' -o -name CMakeLists.txt \) -exec \
        uv -q run --with=cmake-format cmake-lint --suppress-decorations '{}' '+'
}


main() {
    local full="no"
    for arg in $ARGS
    do
        case $arg in
        -h|--help)
            cat <<EOF
Perform static checks on libpqxx build tree.

Usage:
    $0 -h|--help -- print this message and exit.
    $0 -f|--full -- perform full check, including C++ analysis.
    $0 -- perform default check.
EOF
            exit 0
        ;;
        -f|--full)
            full="yes"
        ;;
        *)
            echo >&2 "Unknown argument: '$arg'"
            exit 1
        ;;
        esac
    done

    check_ascii
    pylint
    shelllint
    check_news_version
    check_compiler_internal_headers
    # check_cmake
    mdlint
    check_yaml
    if [ $full == "yes" ]
    then
        cpplint
    fi
}


main
