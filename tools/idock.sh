#! /bin/bash
#
# Run an interactive docker container with the standard options, bind-mounting
# the libpqxx source directory as /mnt/pqxx.
#
# Passes all arguments on to docker, so give, in this order:
#
# 1. Any further command-line options to "docker run", e.g. "--log-level=warn".
# 2. Image name, e.g. "debian:unstable".
# 3. Optional command line to run inside the container, e.g. "ksh".
set -Cue -o pipefail

case "${1:-}" in
    -h|--help|'')
        cat <<EOF >&2
Runs an interactive docker container with less typing.

You can run an optional command line inside the container, or it will run an
interactive shell by default.

Usage:
    $0 [docker options] <image> [command line]

Inside the container, the libpqxx source tree will be bind-mounted at
'/mnt/pqxx'.
EOF
        exit 1
        ;;
esac

# (This trick is bash-specific.  It's surprisingly difficult in pure POSIX to
# just get the location of the current script.)
PQXX="$(dirname "$(dirname "${BASH_SOURCE[0]}")")"
MOUNT=/mnt/pqxx

echo "Running docker with '$PQXX' bind-mounted as '$MOUNT'."

# The actual work.
docker run -ti -v "$PQXX:/mnt/pqxx" "$@"
