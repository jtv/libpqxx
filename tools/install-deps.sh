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
# * archlinux-coverage (for running test coverage analysis)
# * archlinux-lint (for running full lint)
# * debian
# * fedora
# * macos
# * ubuntu
# * ubuntu-codeql (for CodeQL analysis)
# * ubuntu-valgrind (for running Valgrind tests)
# * windows
#
# The script may output shell commands that you'll need to run in your own
# shell process, to set variables and such.
#
# It also logs package installation to /tmp/install.log.

set -Cue -o pipefail


# For Debian-flavoured distros:
export DEBIAN_FRONTEND=noninteractive TZ=UTC


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


# Install packages using apt.  Includes database update.
apt_install() {
    (
        apt-get -q update
        apt-get -qy install "$@"
    ) >>/tmp/install.log
}


# Install packages using Homebrew.
brew_install() {
    brew install --quiet "$@" >>/tmp/install.log
}


# Install packages using dnf.
dnf_install() {
    dnf -qy install "$@" >>/tmp/install.log
}


# Install packages using pacman.  Includes database update.
pacman_install() {
    pacman -qSuy --needed --noconfirm "$@" >>/tmp/install.log
}


PKGS_ALL_AUTOTOOLS=(autoconf autoconf-archive automake libtool)
PKGS_ARCHLINUX_BASE=(diffutils postgresql-libs python3 uv)
PKGS_ARCHLINUX_AUTOTOOLS=("${PKGS_ALL_AUTOTOOLS[@]}" make)
PKGS_DEBIAN_BASE=(libpq-dev postgresql-server-dev-all python3)
PKGS_DEBIAN_AUTOTOOLS=("${PKGS_ALL_AUTOTOOLS[@]}" make)


install_archlinux() {
    pacman_install \
        "${PKGS_ARCHLINUX_BASE[@]}" "${PKGS_ARCHLINUX_AUTOTOOLS[@]}" \
        postgresql which \
        "$(compiler_pkg "$1")"

    echo "export PGHOST=/run/postgresql"
}


# Install test coveraage tools.
install_archlinux_coverage() {
    pacman_install \
        "${PKGS_ARCHLINUX_BASE[@]}" "${PKGS_ARCHLINUX_AUTOTOOLS[@]}" \
        lcov postgresql which \
        "$(compiler_pkg "$1")"

    echo "export PGHOST=/run/postgresql"
}


# Install Facebook's Infer static analysis tool.
install_archlinux_infer() {
    local infer_ver="1.2.0"
    local downloads="https://github.com/facebook/infer/releases/download"
    local tarball="infer-linux-x86_64-v$infer_ver.tar.xz"
    local url="$downloads/v$infer_ver/$tarball"

    if [ "$1" != "g++" ]
    then
        echo >&2 "Facebook's 'infer' only seems to work with g++, not '%1'."
        exit 1
    fi

    pacman_install \
        "${PKGS_ARCHLINUX_BASE[@]}" "${PKGS_ARCHLINUX_AUTOTOOLS[@]}" \
        tzdata wget xz \
        "$(compiler_pkg "$1")"

    cd /opt
    # This is not idempotent.  I wasn't joking about using a disposable
    # environment.
    wget -q "$url" >>/tmp/install.log
    tar -xJf "$tarball" >>/tmp/install.log
    ln -s /opt/infer-*/bin/infer /usr/local/bin/infer
}


# Install for a "lint.sh --full" run (but no actual build).
install_archlinux_lint() {
    pacman_install \
        "${PKGS_ARCHLINUX_BASE[@]}" \
        cmake cppcheck make markdownlint python-pyflakes ruff shellcheck \
        which yamllint \
        "$(compiler_pkg "$1")"
}


install_debian() {
    apt_install \
        "${PKGS_DEBIAN_BASE[@]}" "${PKGS_DEBIAN_AUTOTOOLS[@]}" \
        postgresql \
        "$(compiler_pkg "$1" clang g++)"

    echo "export PGHOST=/tmp"
    echo "export PATH='$PATH:$HOME/.local/bin'"
    echo "export PGBIN='$(ls -d /usr/lib/postgresql/*/bin)/'"
}


install_fedora() {
    dnf_install \
        "${PKGS_ALL_AUTOTOOLS[@]}" \
        libasan libubsan postgresql postgresql-devel postgresql-server \
        python3 uv which \
        "$(compiler_pkg "$1" clang g++)"

    echo "export PGHOST=/tmp"
}


install_macos() {
    # Looks like our compilers come pre-installed on this image.
    local pg_ver=18

    brew_install \
        "${PKGS_ALL_AUTOTOOLS[@]}" \
        postgresql@$pg_ver uv libpq

    echo "export PGHOST=/tmp PGBIN=/opt/homebrew/bin/ PGVER=$pg_ver"
}


install_ubuntu_codeql() {
    # Can't use apt_install here, since we're running it in sudo.
    (
        sudo -E apt-get -q -o DPkg::Lock::Timeout=120 update
        sudo -E apt-get -q install -y -o DPkg::Lock::Timeout=120 \
            cmake git libpq-dev make \
            "$(compiler_pkg "$1" clang g++)"
    ) >>/tmp/install.log
}


install_ubuntu() {
    apt_install \
        "${PKGS_DEBIAN_BASE[@]}" "${PKGS_DEBIAN_AUTOTOOLS[@]}" \
        postgresql \
        "$(compiler_pkg "$1" clang g++)"

    echo "export PGHOST=/tmp"
    echo "export PATH='$PATH:$HOME/.local/bin'"
    echo "export PGBIN='$(ls -d /usr/lib/postgresql/*/bin)/'"
}


install_ubuntu_valgrind() {
    apt_install \
        "${PKGS_DEBIAN_BASE[@]}" \
	cmake ninja-build postgresql valgrind \
        "$(compiler_pkg "$1" clang g++)"

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
    "$msys/usr/bin/bash.exe" -l -c "
(
    # Grok says we may need to let pacman run 2 upgrades.
    pacman -Sy --needed --noconfirm &&
    pacman -Syu --needed --noconfirm ;
) || pacman -Su --needed --noconfirm &&

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
    archlinux-coverage)
        install_archlinux_coverage "$COMPILER"
        ;;
    archlinux-infer)
        install_archlinux_infer "$COMPILER"
        ;;
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
    # Ubuntu, but only for the purpose of running valgrind.
    ubuntu-valgrind)
        install_ubuntu_valgrind "$COMPILER"
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
