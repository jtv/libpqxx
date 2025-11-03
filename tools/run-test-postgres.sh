#! /bin/bash -x
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

if [ -d /run -a ! -d /run/postgresql ]
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
    chown $RUN_AS -- "$PGDATA" "$PGHOST"

    if [ "${MAKE_RUN:-no}" = "yes" ]
    then
        # Assuming privileges.
        chown $RUN_AS /run/postgresql
    fi
fi


CREATEUSER="${PGBIN:-}createuser"
PGCTL="${PGBIN:-}pg_ctl"


# Additional options for initd & postgres.
#
# This is really really stupid.  For some reason these options don't work in
# our macOS and Windows CI setups.  It's at least understandable for macOS
# where we only get postgres 14.  But on Windows we're supposed to have 18 and
# still they don't work.
case "$OSTYPE" in
    darwin*)
        # TODO: Update this once macOS postgres supports our flags.
        INIT_EXTRA=
        POSTGRES_EXTRA=
        ;;
    cygwin|msys|win32)
        # TODO: Update this once Windows postgres supports our flags.
        INIT_EXTRA=
        POSTGRES_EXTRA=
        ;;
    *)
        # (Using short-form options because some BSDs don't support the
	# long-form ones, according to the initdb/postgres man pages.)
        # -N disables sync during init, trading restartability for speed.
        INIT_EXTRA="-A trust -E unicode -N"
	# -F disables fsync, trading restartability for speed.
        POSTGRES_EXTRA="-F"
        ;;
esac

RUN_INITDB="$PGCTL init -D \"$PGDATA\" \
    --options \"--no-instructions $INIT_EXTRA\""
# TODO: Try --single?
RUN_POSTGRES="$PGCTL start -D \"$PGDATA\" -l $LOG \
    --options \"-k \"$PGHOST\" $POSTGRES_EXTRA\""
RUN_CREATEUSER="$CREATEUSER -w -d $ME"


# Log $1 as a big, clearly recognisable banner.
banner() {
    cat >>"$LOG" <<EOF

*** $1 ***

EOF
}


if ! pg_isready --timeout=5
then
    # It does not look as if a cluster exists yet.  Create one.
    if [ "$ME" = "$RUN_AS" ]
    then
        banner "initdb"
        $RUN_INITDB >>"$LOG"
        # Run postgres server in the background.  This is not great practice
        # but...  we're doing this for a disposable environment.
        banner "start postgres"
        $RUN_POSTGRES >>"$LOG"
    else
        # Same thing, but "su" to postgres user.
        banner "initdb"
        su "$RUN_AS" -c "$RUN_INITDB" >>"$LOG"
        banner "start postgres"
        su "$RUN_AS" -c "$RUN_POSTGRES" >>"$LOG"
    fi

    if ! pg_isready --timeout=120
    then
        echo >&2 "ERROR: Database is not ready."
        exit 1
    fi
fi

banner "createuser $ME"

if [ "$ME" = "$RUN_AS" ]
then
    # XXX: Yes but what if it already exists..?
    $RUN_CREATEUSER
else
    su postgres -c "$RUN_CREATEUSER"
fi

banner "createdb $ME"

createdb --template=template0 --encoding=UNICODE "$ME"
