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

# (This trick is bash-specific.  It's surprisingly difficult in pure POSIX to
# just get the location of the current script.)
PQXX="$(dirname $(dirname "$BASH_SOURCE"))"
MOUNT=/mnt/pqxx

echo "Running docker with '$PQXX' bind-mounted as '$MOUNT'."

docker run -ti -v "$PQXX:/mnt/pqxx" $*
