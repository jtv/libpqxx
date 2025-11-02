#! /bin/bash
#
# Start a PostgreSQL server & database for temporary use in tests.
# Creates a database for the current user, with trust authentication.
#
# This is meant to be run in a disposable container or VM.  Run as root.
# First set PGHOST and PGDATA.  The postgres binaries must be in $PATH.
#
# Pass an optional username for a system user with privileged access to the
# cluster.  On a normal Linux install this should be "postgres" but it defaults
# to the current user.
#
# If the PostgreSQL binaries (initdb, createdb etc.) are not in the command
# path, set PGBIN to its location, ending in a trailing slash.

set -Cue -o pipefail

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
ME="$(whoami)"
RUN_AS="${1:-$ME}"

PGBIN="${PGBIN:-}"

mkdir -p -- "$PGDATA" "$PGHOST"
if [ "$ME" != "$RUN_AS" ]
then
    chown postgres -- "$PGDATA" "$PGHOST"
fi


# Look up commands' locations now, because once we're inside a "su"
# environment, they may not be in our PATH.
INITDB="${PGBIN:-}initdb"
CREATEUSER="${PGBIN:-}createuser"
POSTGRES="${PGBIN:-}postgres"

# Since this is a disposable environment, we don't need the server to spend
# any time ensuring that data is persistently stored.
RUN_INITDB="$INITDB --pgdata $PGDATA --auth trust --nosync"
RUN_POSTGRES="$POSTGRES -D $PGDATA -k $PGHOST"
RUN_CREATEUSER="$CREATEUSER -w -d $ME"


# Log $1 as a big, clearly recognisable banner.
banner() {
    cat >>"$LOG" <<EOF

*** $1 ***

EOF
}


if [ "$ME" = "$RUN_AS" ]
then
    banner "initdb"
    $RUN_INITDB >>"$LOG"
    # Run postgres server in the background.  This is not great practice but...
    # we're doing this for a disposable environment.
    banner "start postgres"
    $RUN_POSTGRES >>"$LOG" &
else
    # Same thing, but "su" to different user.
    banner "initdb"
    su "$RUN_AS" -c "$RUN_INITDB" >>"$LOG"
    banner "start postgres"
    su "$RUN_AS" -c "$RUN_POSTGRES" >>"$LOG" &
fi

# Wait for postgres to become available.
# TODO: Set tighter deadline than CircleCI's.
while ! pg_isready -U "$RUN_AS"
do
    sleep .1
done

banner "createuser $ME"

if [ "$ME" = "$RUN_AS" ]
then
    $RUN_CREATEUSER
else
    su "$RUN_AS" -c "$RUN_CREATEUSER"
fi

banner "createdb $ME"

createdb --template=template0 --encoding=UNICODE "$ME"
