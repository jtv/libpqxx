/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/internal/libpq-forward.hxx
 *
 *   DESCRIPTION
 *      Minimal forward declarations of libpq types needed in libpqxx headers
 *      DO NOT INCLUDE THIS FILE when building client programs.
 *
 * Copyright (c) 2005-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
extern "C"
{
struct pg_conn;
struct pg_result;
struct pgNotify;
}

namespace pqxx
{
namespace internal
{
/// Forward declarations of libpq types as needed in libpqxx headers
namespace pq
{
typedef pg_conn PGconn;
typedef pg_result PGresult;
typedef pgNotify PGnotify;
typedef void (*PQnoticeProcessor)(void *, const char *);
}
}

/// PostgreSQL database row identifier
typedef unsigned int oid;
}

