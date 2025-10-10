#! /usr/bin/bash

# Install packages we need to get a full test run working.
#
# Must be run with root privileges.  This is meant for use in disposable
# containers and VMs.
#
# Usage: install-deps.sh <system>
#
# Where <system> is one of the environments for which this script works:
# * debian
# * debian-lint (for running full lint)
# * archlinux
#
# The script may output shell commands that you'll need to run in your own
# shell process, to set variables and such.

set -euC -o pipefail

install_debian() {
    local pgbin

    apt-get -q update >/dev/null

    # Really annoying: there's no package for uv as of yet, so we need to
    # install pipx just so we can use that to install uv.
    DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        build-essential autoconf autoconf-archive automake cppcheck clang \
        libpq-dev lsb-release markdownlint python3 postgresql \
        postgresql-server-dev-all shellcheck libtool pipx yamllint >/dev/null
    pipx install uv >/dev/null

    pgbin="$(ls -d /usr/lib/postgresql/*/bin)"

    echo "export PATH='$PATH:$HOME/.local/bin:$pgbin'"
}


install_debian_lint() {
    apt-get -q update >/dev/null

    # Really annoying: there's no package for uv as of yet, so we need to
    # install pipx just so we can use that to install uv.
    DEBIAN_FRONTEND=noninteractive TZ=UTC apt-get -q install -y \
        build-essential cmake cppcheck clang clang-tidy libpq-dev \
        markdownlint postgresql postgresql-server-dev-all shellcheck yamllint \
        pipx >/dev/null
    pipx install uv >/dev/null

    echo "export PATH='$PATH:$HOME/.local/bin'"
}


install_archlinux() {
    pacman --quiet --noconfirm -Sy >/dev/null
    pacman --quiet --noconfirm -S \
        autoconf autoconf-archive automake clang cmake cppcheck diffutils \
        libtool make postgresql postgresql-libs python3 shellcheck uv \
        yamllint >/dev/null
}


if test -z "${1:-}"
then
    echo >&2 "Pass profile name, e.g. 'debian' or 'archlinux'."
    exit 1
fi

case "$1" in
    debian)
        install_debian
        ;;
    debian-lint)
        install_debian_lint
        ;;

    archlinux)
        install_archlinux
        ;;

    *)
        echo >&2 "$0 does not support $1."
        exit 1
        ;;
esac

echo "export PGHOST=/tmp PGDATA=/db"
