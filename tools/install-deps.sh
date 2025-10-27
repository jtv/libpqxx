#! /bin/bash

# Install packages we need to get a full test run working.
#
# Must be run with root privileges.  This is meant for use in disposable
# containers and VMs.
#
# Usage: install-deps.sh <system>
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

install_archlinux() {
    pacman --quiet --noconfirm -Sy >>/tmp/install.log
    pacman --quiet --noconfirm -S \
        autoconf autoconf-archive automake clang cmake cppcheck diffutils \
        libtool make postgresql postgresql-libs python3 shellcheck uv \
        which yamllint >>/tmp/install.log
    echo '# (No settings needed.)'
}


install_archlinux_lint() {
    pacman --quiet --noconfirm -Sy >>/tmp/install.log
    pacman --quiet --noconfirm -S \
        clang cmake cppcheck diffutils make postgresql-libs python3 \
        shellcheck uv which yamllint >>/tmp/install.log
    echo '# (No settings needed.)'
}


install_debian() {
    local pgbin

    apt-get -q update >>/tmp/install.log

    # Really annoying: there's no package for uv as of yet, so we need to
    # install pipx just so we can use that to install uv.
    DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        build-essential autoconf autoconf-archive automake cppcheck clang \
        libpq-dev markdownlint python3 postgresql postgresql-server-dev-all \
        shellcheck libtool pipx yamllint >>/tmp/install.log
    pipx install uv >>/tmp/install.log

    pgbin="$(ls -d /usr/lib/postgresql/*/bin)"

    PATH="$PATH:$pgbin" which -a initdb # XXX: DEBUG CODE

    echo "PATH='$PATH:$HOME/.local/bin:$pgbin'"
    echo "export PATH"
}


install_debian_lint() {
    apt-get -q update >>/tmp/install.log

    # Really annoying: there's no package for uv as of yet, so we need to
    # install pipx just so we can use that to install uv.
    DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        build-essential cmake cppcheck clang clang-tidy libpq-dev \
        markdownlint postgresql postgresql-server-dev-all shellcheck yamllint \
        pipx >>/tmp/install.log
    pipx install uv >>/tmp/install.log

    echo "PATH='$PATH:$HOME/.local/bin'"
    echo "export PATH"
}


install_fedora() {
    dnf -qy install \
        autoconf autoconf-archive automake cppcheck clang libasan libtool \
        libubsan postgresql postgresql-devel postgresql-server shellcheck uv \
        yamllint \
        >>/tmp/install.log

    # I haven't found a curated package for Markdownlint (mdl) on Fedora.
    # That's fine: we run it on the other systems.  Just stub it out.
    echo "alias mdl='echo mdl'"
}


install_macos() {
    brew install --quiet \
        autoconf autoconf-archive automake cppcheck libtool postgresql \
        shellcheck uv yamllint libpq >>/tmp/install.log
}


install_ubuntu_codeql() {
    sudo apt-get -q update >>/tmp/install.log

    sudo DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        clang cmake git libpq-dev make >>/tmp/install.log
    echo '# (No settings needed.)'
}


# TODO: Install versionless "postgresql"?  May have to pass password globally.
# XXX: This may help: https://community.chocolatey.org/packages/postgresql
install_windows() {
    local pf="/c/Program Files"
    local cmake_bin="$pf/CMake/bin"
    local llvm_bin="$pf/llvm/bin"

    # For unclear reason, g++ doesn't seem to work.  But to install it you'd
    # ask for mingw.
    # TODO: Would be good to have this working.
    # local pd="/c/ProgramData"
    # local mingw_bin="$pd/mingw64/mingw64/bin"

    # TODO: Couldn't get PostgreSQL running on Windows in CircleCI.
    # local pg_bin="$pf/PostgreSQL/16/bin"

    # This dumps an unacceptable amount of garbage to stderr, even with the
    # --limit-output option which AFAICT does nothing to limit output (and
    # why is there no --quiet option?).
    #
    # But if we let this run quietly, then it times out.  And we can't let the
    # output go to stdout because that's where we write our variables, so we
    # let it generate progress information and send the output to stderr.
    choco install cmake llvm mingw ninja postgresql16 --limit-output -y \
        1>&2 | tee install.log >&2

    # This is just useless...  To get the installed commands in your path,
    # you run refreshenv.exe AND THEN CLOSE THE SHELL AND OPEN A NEW ONE.
    # Instead, we'll just have to add all these directories to PATH.
    echo "PATH='$PATH:$cmake_bin:$llvm_bin'"
    echo "export PATH"
}


if test -z "${1:-}"
then
    echo >&2 "Pass profile name, e.g. 'debian' or 'archlinux'."
    exit 1
fi

case "$1" in
    archlinux)
        install_archlinux
        ;;
    # Arch system, but only for the purpose of running "lint --full".
    # (We only need to do that on one of the systems.)
    archlinux-lint)
        install_archlinux_lint
        ;;

    debian)
        install_debian
        ;;

    fedora)
        install_fedora
        ;;

    macos)
        install_macos
        ;;

    ubuntu)
        # Same as for Debian!
        install_debian
        ;;
    # Ubuntu system, but only for the purpose of running a CodeQL scan.
    ubuntu_codeql)
        install_ubuntu_codeql
        ;;

    windows)
        install_windows
        ;;

    *)
        echo >&2 "$0 does not support $1."
        exit 1
        ;;
esac

echo "PGDATA=/tmp/db"
echo "PGHOST=/tmp"
echo "export PGDATA PGHOST"
