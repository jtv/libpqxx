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

PQXX="$(dirname $(dirname "$BASH_SOURCE"))"
docker run -ti -v "$PQXX:/mnt/pqxx" $*
