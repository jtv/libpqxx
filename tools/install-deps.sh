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

set -Cue -o pipefail


# Common case: OS package we need to install to get compiler $1.
#
# If $1 equals "clang++", print $2 (or "clang" by default).  If $1 equals
# "g++", print $3 (or "gcc" by default).  Otherwise, fail.
compiler_pkg() {
    local compiler="$1"
    case "$1" in
        clang++)
            echo "${2:-clang}"
            ;;
        g++)
            echo "${3:-gcc}"
            ;;
        *)
            echo >&2 "Unsupported compiler: '$compiler'."
            exit 1
            ;;
    esac
}


install_archlinux() {
    local cxxpkg
    cxxpkg="$(compiler_pkg "$1" clang gcc)"

    (
        pacman --quiet --noconfirm -Sy
        pacman --quiet --needed --noconfirm -S \
            autoconf autoconf-archive automake diffutils libtool make \
            postgresql postgresql-libs python3 uv which \
            "$cxxpkg"
    ) >>/tmp/install.log

    echo "export PGHOST=/run/postgresql"
}


# Install Facebook's Infer static analysis tool.
install_archlinux_infer() {
    local infer_ver="1.2.0"
    local downloads="https://github.com/facebook/infer/releases/download"
    local tarball="infer-linux-x86_64-v$infer_ver.tar.xz"
    local url="$downloads/v$infer_ver/$tarball"
    local cxxpkg

    if [ "$1" != "g++" ]
    then
        echo >&2 "Facebook's 'infer' only seems to work with g++, not '%1'."
        exit 1
    fi

    cxxpkg="$(compiler_pkg "$1" clang gcc)"

    (
        pacman --quiet --noconfirm -Sy
        pacman --quiet --needed --noconfirm -S \
            autoconf autoconf-archive automake diffutils libtool make \
            postgresql-libs python3 tzdata uv wget xz \
            "$cxxpkg"
    ) >>/tmp/install.log

    cd /opt
    # This is not idempotent.  I wasn't joking about using a disposable
    # environment.
    wget -q "$url" >>/tmp/install.log
    tar -xJf "$tarball" >>/tmp/install.log
    ln -s /opt/infer-*/bin/infer /usr/local/bin/infer
}


install_archlinux_lint() {
    local cxxpkg
    cxxpkg="$(compiler_pkg "$1" clang gcc)"

    (
        pacman --quiet --noconfirm -Sy >>/tmp/install.log
        pacman --quiet --noconfirm -S \
            cmake cppcheck diffutils make markdownlint postgresql-libs python3 \
            python-pyflakes ruff shellcheck uv which yamllint \
            "$cxxpkg"
    ) >>/tmp/install.log
}


install_debian() {
    local cxxpkg
    cxxpkg="$(compiler_pkg "$1")"

    (
        apt-get -q update

        # Really annoying: there's no package for uv as of yet, so we need to
        # install pipx just so we can use that to install uv.
        DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
            build-essential autoconf autoconf-archive automake libpq-dev \
            python3 postgresql postgresql-server-dev-all libtool pipx \
            "$cxxpkg"

        # We need pipx only to install uv.  :-(
        # TODO: Once uv has been packaged, get rid of pipx.
        pipx install uv
    ) >> /tmp/install.log

    echo "export PGHOST=/tmp"
    echo "export PATH='$PATH:$HOME/.local/bin'"
    echo "export PGBIN='$(ls -d /usr/lib/postgresql/*/bin)/'"
}


install_fedora() {
    local cxxpkg
    cxxpkg="$(compiler_pkg "$1" clang g++)"
    dnf -qy install \
        autoconf autoconf-archive automake libasan libtool libubsan \
        postgresql postgresql-devel postgresql-server python3 uv which \
        "$cxxpkg" \
        >>/tmp/install.log

    echo "export PGHOST=/tmp"
}


install_macos() {
    # Looks like our compilers come pre-installed on this image.
    local pg_ver=18

    brew install --quiet \
        autoconf autoconf-archive automake libtool postgresql@$pg_ver uv \
        libpq \
        >>/tmp/install.log

    echo "export PGHOST=/tmp PGBIN=/opt/homebrew/bin/ PGVER=$pg_ver"
}


install_ubuntu_codeql() {
    local cxxpkg
    cxxpkg="$(compiler_pkg "$1")"
    (
        sudo apt-get -q -o DPkg::Lock::Timeout=120 update
        sudo DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get \
            -q install -y -o DPkg::Lock::Timeout=120 \
            cmake git libpq-dev make \
            "$cxxpkg"
    ) >>/tmp/install.log
}


install_ubuntu() {
    local cxxpkg
    cxxpkg="$(compiler_pkg "$1")"

    (
        apt-get -q update

        # Really annoying: there's no package for uv as of yet, so we need to
        # install pipx just so we can use that to install uv.
        DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
            build-essential autoconf autoconf-archive automake libpq-dev \
            python3 postgresql postgresql-server-dev-all libtool pipx "$cxxpkg"

        # We need pipx only to install uv.  :-(
        # TODO: Once uv has been packaged, get rid of pipx.
        pipx install uv
    ) >>/tmp/install.log

    echo "export PGHOST=/tmp"
    echo "export PATH='$PATH:$HOME/.local/bin'"
    echo "export PGBIN='$(ls -d /usr/lib/postgresql/*/bin)/'"
}


install_windows() {
    local arch="mingw-w64-x86_64"
    local cxxpkg
    local msys="/C/tools/msys64"
    local mingw="$msys/mingw64"

    cxxpkg="$(compiler_pkg "$1")"
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
    # TODO: Can we speed this up with --needed?
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
" | tee -a install.log >&2

    echo "export PGHOST=localhost"
    echo "export PATH='$PATH'"
    echo "export PGBIN='$mingw/bin/'"
}


if [ -z "${1:-}" ] || [ -z "${2:-}" ]
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
        install_archlinux "$COMPILER"
        ;;
    # Arch system, but only for the purpose of running Facebook's "infer"
    # static analysis tool.
    archlinux-infer)
        install_archlinux_infer "$COMPILER"
        ;;
    # Arch system, but only for the purpose of running "lint.sh --full".
    # (We only need to do that on one of the systems.)
    archlinux-lint)
        install_archlinux_lint "$COMPILER"
        ;;

    debian)
        install_debian "$COMPILER"
        ;;

    fedora)
        install_fedora "$COMPILER"
        ;;

    macos)
        install_macos "$COMPILER"
        ;;

    ubuntu)
        install_ubuntu "$COMPILER"
        ;;
    # Ubuntu system, but only for the purpose of running a CodeQL scan.
    ubuntu_codeql)
        install_ubuntu_codeql "$COMPILER"
        ;;

    windows)
        install_windows "$COMPILER"
        ;;

    *)
        echo >&2 "$0 does not support $1."
        exit 1
        ;;
esac

echo "export PGDATA=/tmp/db"

# Skip routine lint check except in the actual lint run.
case "$PROFILE" in
    *lint)
        # We're doing a lint check, which is actually the default.
        # But we only need 1 job per run to do this.
        ;;
    *)
        # Regular job.  Skip redundant lint check.
        echo "export PQXX_LINT=skip"
        ;;
esac
