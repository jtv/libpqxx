#! /bin/bash
#
# Start a PostgreSQL server & database for temporary use in tests.
# Creates a database for the current user, with trust authentication.
#
# THIS IS NOT A GENERIC UTILITY.  It's meant just to get a database running on
# a disposable VM in our specific CI.
#
# Run as root.  First set PGHOST and PGDATA.  The postgres binaries must be in
# $PATH.  Pass an optional username for a system user with privileged access to
# the cluster.  On a normal Linux install this should be "postgres" but it
# defaults to the current user.
#
# If the PostgreSQL binaries (initdb, createdb etc.) are not in the command
# path, set PGBIN to their location, ending in a trailing slash.
#
# If pg_ctl and pg_isready have a release suffix (e.g. pg_ctl-18) as is the
# case with Homebrew, set PGVER to the release number (e.g. "18").

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

mkdir -p -- "$PGDATA" "$PGHOST"

if [ -d /run ] && [ ! -d /run/postgresql ]
then
    MAKE_RUN=yes
    mkdir -p /run/postgresql
fi

touch $LOG
if [ "$ME" != "$RUN_AS" ]
then
    # Must be writable to both $ME an $RUN_AS.
    chmod a+w $LOG

    # Assuming privileges.
    chown "$RUN_AS" -- "$PGDATA" "$PGHOST"

    if [ "${MAKE_RUN:-no}" = "yes" ]
    then
        # Assuming privileges.
        chown "$RUN_AS" /run/postgresql
    fi
fi


# If PGVER is set, append it to $1.
#
# This is something annoying that the Homebrew postgres package does.
add_version_suffix() {
    local ver="${PGVER:-}"
    local executable="$1"
    local suffixed="$executable-$ver"

    if [ -n "$ver" ] && [ -x "$suffixed" ]
    then
        echo "$suffixed"
    else
        echo "$executable"
    fi
}


# Add optional path & release suffix to a postgres binary's name.
adorn_bin() {
    add_version_suffix "${PGBIN:-}$1"
}


CREATEDB="$(adorn_bin createdb)"
CREATEUSER="$(adorn_bin createuser)"
PGCTL="$(adorn_bin pg_ctl)"
PGISREADY="$(adorn_bin pg_isready)"
PSQL="$(adorn_bin psql)"

if $PSQL -c "SELECT 'Database already works.'"
then
    exit 0
fi


if [ "$PGHOST" = "localhost" ]
then
    # This is the case for Windows, where there's no Unix domain sockets.
    SOCKDIR=
else
    # Unix-like systems can use the local socket to connect to postgres.
    # Pass this option directly to postgres, using pg_ctl's -o.
    SOCKDIR="-o-k$PGHOST"
fi

# -o passes options to initdb.
# -o-N disables sync during init, trading restartability for speed.
RUN_INITDB="$PGCTL init \
    -D $PGDATA -o-Atrust -o--no-instructions -o-N -o-EUTF8"
# -o-F disables fsync, trading restartability for speed.
RUN_POSTGRES="$PGCTL start -D $PGDATA -l $LOG -o-F $SOCKDIR"
RUN_CREATEUSER="$CREATEUSER -w -d $ME"


# Log $1 as a big, clearly recognisable banner.
banner() {
    cat <<EOF

*** $1 ***

EOF
}


if ! $PGISREADY --timeout=5
then
    # It does not look as if a cluster exists yet.  Create one.
    if [ "$RUN_AS" = "$ME" ]
    then
        banner "initdb"
        $RUN_INITDB
        # Run postgres server in the background.  This is not great practice
        # but...  we're doing this for a disposable environment.
        banner "start postgres"
	# We need to redirect I/O for Windows' sake.  Without it, on that
	# platform, this script will complete but never exit.
        $RUN_POSTGRES \
	    >postgres-start-stdout.log 2>postgres-start-stderr.log \
	    </dev/null
    else
        # Same thing, but "su" to postgres user.
        banner "initdb"
        su "$RUN_AS" -c "$RUN_INITDB"
        banner "start postgres"
        su "$RUN_AS" -c "$RUN_POSTGRES"
    fi

    if ! $PGISREADY --timeout=120
    then
        echo >&2 "ERROR: Database is not ready."
        exit 1
    fi
fi

banner "createuser $ME"

if [ "$RUN_AS" != "$ME" ]
then
    # TODO: Better check for role presence.
    if ! $PSQL --host="$PGHOST" -c "SELECT 'New database user can log in.'"
    then
        su "$RUN_AS" -c "$RUN_CREATEUSER"
    fi
fi


banner "Checking for database $ME"
if ! $PSQL --list | grep "^$ME$"
then
    banner "createdb $ME"
    $CREATEDB "$ME"
fi
