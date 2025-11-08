#! /bin/bash

# Install packages we need to get a full test run working.
#
# Must be run with root privileges.  This is meant for use in disposable
# containers and VMs.
#
# Usage: install-deps.sh <system> <compiler>
#
# Where <system> is one of the environments for which this script works:
# * archlinux
# * debian
# * debian-lint (for running full lint)
# * fedora
# * macos
# * windows
#
# The script may output shell commands that you'll need to run in your own
# shell process, to set variables and such.
#
# It also logs package installation to /tmp/install.log.

# XXX: POSIX allows convenient "export A=1 B=2 C=3".

set -Cue -o pipefail


# Common case: OS package we need to install to get compiler $1.
#
# If $1 equals "clang++", print $2 (or "clang" by default).  If $1 equals
# "g++", print $3 (or empty string by default).  Otherwise, fail.
compiler_pkg() {
    case "$1" in
        clang++)
            echo "${2:-clang}"
            ;;
        g++)
            echo "${3:-}"
            ;;
        *)
            echo >&2 "Unsupported compiler: '$compiler'."
            exit 1
            ;;
    esac
}


install_archlinux() {
    local cxxpkg="$(compiler_pkg $1 clang gcc)"

    pacman --quiet --noconfirm -Sy >>/tmp/install.log
    pacman --quiet --noconfirm -S \
        autoconf autoconf-archive automake diffutils libtool make postgresql \
        postgresql-libs python3 uv which $cxxpkg \
        >>/tmp/install.log

    echo "PGHOST=/run/postgresql"
    echo "export PGHOST"
}


install_archlinux_lint() {
    local cxxpkg="$(compiler_pkg $1)"

# TODO: Set up Infer.  https://fbinfer.com/docs/getting-started/
# TODO: Set up Markdownlint (mdl).
    pacman --quiet --noconfirm -Sy >>/tmp/install.log
    pacman --quiet --noconfirm -S \
        cmake cppcheck diffutils make postgresql-libs python3 \
        shellcheck uv which yamllint $cxxpkg >>/tmp/install.log
}


install_debian() {
    local cxxpkg="$(compiler_pkg $1)"
    local pgbin

    apt-get -q update >>/tmp/install.log

    # Really annoying: there's no package for uv as of yet, so we need to
    # install pipx just so we can use that to install uv.
    DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        build-essential autoconf autoconf-archive automake libpq-dev \
        python3 postgresql postgresql-server-dev-all libtool pipx \
        $cxxpkg >>/tmp/install.log

    # We need pipx only to install uv.  :-(
    # TODO: Once uv has been packaged, get rid of pipx.
    pipx install uv >>/tmp/install.log

    pgbin="$(ls -d /usr/lib/postgresql/*/bin)"

    echo "PGHOST=/tmp"
    echo "export PGHOST"
    echo "PATH='$PATH:$HOME/.local/bin'"
    echo "export PATH"
    echo "PGBIN='$(ls -d /usr/lib/postgresql/*/bin)/'"
    echo "export PGBIN"
}


install_fedora() {
    local cxxpkg="$(compiler_pkg $1 clang g++)"
    dnf -qy install \
        autoconf autoconf-archive automake libasan libtool libubsan \
        postgresql postgresql-devel postgresql-server python3 uv which \
        $cxxpkg \
        >>/tmp/install.log

    echo "PGHOST=/tmp"
    echo "export PGHOST"
}


install_macos() {
    # Looks like our compilers come pre-installed on this image.
    local pg_ver=18

    brew install --quiet \
        autoconf autoconf-archive automake libtool postgresql@$pg_ver uv \
        libpq \
        >>/tmp/install.log

    echo "PGHOST=/tmp"
    echo "export PGHOST"
    echo "PGBIN='/opt/homebrew/bin/'"
    echo "export PGBIN"
    echo PGVER=$pg_ver
    echo "export PGVER"
}


install_ubuntu_codeql() {
    local cxxpkg="$(compiler_pkg $1)"
    sudo apt-get -q -o DPkg::Lock::Timeout=120 update >>/tmp/install.log

    sudo DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get \
        -q install -y -o DPkg::Lock::Timeout=120 \
        cmake git libpq-dev make $cxxpkg >>/tmp/install.log
}


install_ubuntu() {
    local cxxpkg="$(compiler_pkg $1)"
    local pgbin

    apt-get -q update >>/tmp/install.log

    # Really annoying: there's no package for uv as of yet, so we need to
    # install pipx just so we can use that to install uv.
    DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        build-essential autoconf autoconf-archive automake libpq-dev python3 \
        postgresql postgresql-server-dev-all libtool pipx $cxxpkg \
        >>/tmp/install.log

    # We need pipx only to install uv.  :-(
    # TODO: Once uv has been packaged, get rid of pipx.
    pipx install uv >>/tmp/install.log

    pgbin="$(ls -d /usr/lib/postgresql/*/bin)"

    echo "PGHOST=/tmp"
    echo "export PGHOST"
    echo "PATH='$PATH:$HOME/.local/bin'"
    echo "export PATH"
    echo "PGBIN='$(ls -d /usr/lib/postgresql/*/bin)/'"
    echo "export PGBIN"
}


# TODO: Set up CircleCI caching for package installs.
install_windows() {
    local arch="mingw-w64-x86_64"
    local cxxpkg="$(compiler_pkg $1)"
    local msys="/C/tools/msys64"
    local mingw="$msys/mingw64"

    if [ -n "$cxxpkg" ]
    then
        cxxpkg="$arch-$cxxpkg"
    fi

    # This dumps an unacceptable amount of garbage to stderr, even with the
    # --limit-output option which AFAICT does nothing to limit output (and
    # why is there no --quiet option?).
    #
    # But if we let this run quietly, then it times out.  And we can't let the
    # output go to stdout because that's where we write our variables.  So we
    # let it generate progress information and send the output to stderr.  :-/
    choco install msys2 --limit-output --stoponfirstfailure -y 2>&1 |
        tee -a install.log >&2

    # We need the msys version of libpq, because the one we'd get from choco
    # is for MSVC (odd, for a C library).  Ironically Microsoft's own vcpkg is
    # supposed to have a libpq that works with gcc, but it failed to install.
    #
    # The order matters.  We must prefer MinGW binaries over MSYS ones, or we
    # get weird problems like <cstdlib> failing as it tries to include
    # <stdlib.h>.
    export PATH="$mingw/bin:$msys:$msys/usr/bin:$PATH"

    # Now bootstrap the rest using the MSYS shell.
    "$msys/usr/bin/bash.exe" -l -c "
(
    # Grok says we may need to let pacman run 2 upgrades.
    pacman -Sy --noconfirm &&
    pacman -Syu --noconfirm ;
) || pacman -Su --noconfirm &&

pacman -S \
    $arch-cmake \
    $arch-postgresql \
    $arch-toolchain \
    cmake \
    ninja \
    $cxxpkg \
    --noconfirm
" 2>&1 | tee -a install.log >&2

    echo "PGHOST=localhost"
    echo "export PGHOST"
    echo "PATH='$PATH'"
    echo "export PATH"
    echo "PGBIN='$mingw/bin/'"
    echo "export PGBIN"
}


if [ -z "${1:-}" -o -z "${2:-}" ]
then
    cat >&2 <<EOF
Usage: $0 <profile> <compiler>

Where <profile> is usually an OS name, sometimes a combination of OS and
purpose: archlinux, archilinux-lint, debian, fedora, macos, windows...

And <compiler> is the compiler: g++ or clang++ (or in future perhaps more).
EOF
    exit 1
fi

PROFILE="$1"
COMPILER="$2"

case "$PROFILE" in
    archlinux)
        install_archlinux $COMPILER
        ;;
    # Arch system, but only for the purpose of running "lint --full".
    # (We only need to do that on one of the systems.)
    archlinux-lint)
        install_archlinux_lint $COMPILER
        ;;

    debian)
        install_debian $COMPILER
        ;;

    fedora)
        install_fedora $COMPILER
        ;;

    macos)
        install_macos $COMPILER
        ;;

    ubuntu)
        install_ubuntu $COMPILER
        ;;
    # Ubuntu system, but only for the purpose of running a CodeQL scan.
    ubuntu_codeql)
        install_ubuntu_codeql $COMPILER
        ;;

    windows)
        install_windows $COMPILER
        ;;

    *)
        echo >&2 "$0 does not support $1."
        exit 1
        ;;
esac

echo "PGDATA=/tmp/db"
echo "export PGDATA"

# Skip routine lint check except in the actual lint run.
case "$PROFILE" in
    *lint)
        # We're doing a lint check, which is actually the default.
        # But we only need 1 job per run to do this.
        ;;
    *)
        # Regular job.  Skip redundant lint check.
        echo "PQXX_LINT=skip"
        echo "export PQXX_LINT"
        ;;
esac
