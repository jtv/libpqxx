#! /bin/bash
#
# Start a PostgreSQL server & database for temporary use in tests.
# Creates a database for the current user, with trust authentication.
#
# This is meant to be run in a disposable container or VM.  Run as root.
# First set PGHOST and PGDATA.  The postgres binaries must be in $PATH.

set -euC -o pipefail

if test -z "${PGDATA:-}"
then
    echo >&2 "Set PGDATA."
    exit 1
fi

if test -z "${PGHOST:-}"
then
    echo >&2 "Set PGHOST."
    exit 1
fi

LOG="postgres.log"
USER="$(whoami)"

mkdir -p -- "$PGDATA" "$PGHOST"
chown postgres -- "$PGDATA" "$PGHOST"

# Since this is a disposable environment, we don't need the server to spend
# any time ensuring that data is persistently stored.
su postgres -c "initdb --pgdata \"$PGDATA\" --auth trust --nosync" >>"$LOG"

# Run postgres server in the background.  This is not great practice but...
# we're doing this for a disposable environment.
su postgres -c "postgres -D \"$PGDATA\" -k \"$PGHOST\" " >>"$LOG" &

# XXX: Wait until ready!
sleep 5

su postgres -c "createuser -w -d \"$USER\"" 
createdb --template=template0 --encoding=UNICODE "$USER"
