Libpqxx is a client library for PostgreSQL.  It's a wrapper for libpq.

It assumes that it's talking to a genuine and fully patched libpq, which in
turn connects to a genuine and fully patched PostgreSQL server.  It also
assumes that the application calling it is not crafted to fool libpqxx, but
that inputs or data stored inside the database may be.
