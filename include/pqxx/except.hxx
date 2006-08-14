/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/except.hxx
 *
 *   DESCRIPTION
 *      definition of libpqxx exception classes
 *   pqxx::sql_error, pqxx::broken_connection, pqxx::in_doubt_error, ...
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/except instead.
 *
 * Copyright (c) 2003-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <stdexcept>

#include "pqxx/util"


namespace pqxx
{

/**
 * @addtogroup exception Exception classes
 *
 * These exception classes follow, roughly, the two-level hierarchy defined by
 * the PostgreSQL error codes (see Appendix A of the PostgreSQL documentation
 * corresponding to your server version).  The hierarchy given here is, as yet,
 * not a complete mirror of the error codes.  There are some other differences
 * as well, e.g. the error code statement_completion_unknown has a separate
 * status in libpqxx as in_doubt_error, and too_many_connections is classified
 * as a broken_connection rather than a subtype of insufficient_resources.
 *
 * @see http://www.postgresql.org/docs/8.1/interactive/errcodes-appendix.html
 *
 * @{
 */

/// Exception class for lost or failed backend connection.
class PQXX_LIBEXPORT broken_connection : public PGSTD::runtime_error
{
public:
  broken_connection();
  explicit broken_connection(const PGSTD::string &);
};


/// Exception class for failed queries.
/** Carries a copy of the failed query in addition to a regular error message */
class PQXX_LIBEXPORT sql_error : public PGSTD::runtime_error
{
  PGSTD::string m_Q;

public:
  sql_error();
  explicit sql_error(const PGSTD::string &);
  sql_error(const PGSTD::string &, const PGSTD::string &Q);
  virtual ~sql_error() throw ();

  /// The query whose execution triggered the exception
  const PGSTD::string &query() const throw ();				//[t56]
};


// TODO: should this be called statement_completion_unknown!?
/// "Help, I don't know whether transaction was committed successfully!"
/** Exception that might be thrown in rare cases where the connection to the
 * database is lost while finishing a database transaction, and there's no way
 * of telling whether it was actually executed by the backend.  In this case
 * the database is left in an indeterminate (but consistent) state, and only
 * manual inspection will tell which is the case.
 */
class PQXX_LIBEXPORT in_doubt_error : public PGSTD::runtime_error
{
public:
  explicit in_doubt_error(const PGSTD::string &);
};


/// Internal error in libpqxx library
class PQXX_LIBEXPORT internal_error : public PGSTD::logic_error
{
public:
  explicit internal_error(const PGSTD::string &);
};


/// Database feature not supported in current setup
class PQXX_LIBEXPORT feature_not_supported : public sql_error
{
public:
  explicit feature_not_supported(const PGSTD::string &err) : sql_error(err) {}
  feature_not_supported(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

/// Error in data provided to SQL statement
class PQXX_LIBEXPORT data_exception : public sql_error
{
public:
  explicit data_exception(const PGSTD::string &err) : sql_error(err) {}
  data_exception(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

class PQXX_LIBEXPORT integrity_constraint_violation : public sql_error
{
public:
  explicit integrity_constraint_violation(const PGSTD::string &err) :
	sql_error(err) {}
  integrity_constraint_violation(const PGSTD::string &err,
  	const PGSTD::string &Q) :
	sql_error(err, Q) {}
};


class PQXX_LIBEXPORT invalid_cursor_state : public sql_error
{
public:
  explicit invalid_cursor_state(const PGSTD::string &err) : sql_error(err) {}
  invalid_cursor_state(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

class PQXX_LIBEXPORT invalid_sql_statement_name : public sql_error
{
public:
  explicit invalid_sql_statement_name(const PGSTD::string &err) :
	sql_error(err) {}
  invalid_sql_statement_name(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

class PQXX_LIBEXPORT invalid_cursor_name : public sql_error
{
public:
  explicit invalid_cursor_name(const PGSTD::string &err) : sql_error(err) {}
  invalid_cursor_name(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

class PQXX_LIBEXPORT syntax_error : public sql_error
{
public:
  explicit syntax_error(const PGSTD::string &err) : sql_error(err) {}
  syntax_error(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

class PQXX_LIBEXPORT undefined_table : public syntax_error
{
public:
  explicit undefined_table(const PGSTD::string &err) : syntax_error(err) {}
  undefined_table(const PGSTD::string &err, const PGSTD::string &Q) :
    syntax_error(err, Q) {}
};

class PQXX_LIBEXPORT insufficient_privilege : public sql_error
{
public:
  explicit insufficient_privilege(const PGSTD::string &err) : sql_error(err) {}
  insufficient_privilege(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

/// Resource shortage on the server
class PQXX_LIBEXPORT insufficient_resources : public sql_error
{
public:
  explicit insufficient_resources(const PGSTD::string &err) : sql_error(err) {}
  insufficient_resources(const PGSTD::string &err, const PGSTD::string &Q) :
	sql_error(err,Q) {}
};

class PQXX_LIBEXPORT disk_full : public insufficient_resources
{
public:
  explicit disk_full(const PGSTD::string &err) : insufficient_resources(err) {}
  disk_full(const PGSTD::string &err, const PGSTD::string &Q) :
	insufficient_resources(err,Q) {}
};

class PQXX_LIBEXPORT out_of_memory : public insufficient_resources
{
public:
  explicit out_of_memory(const PGSTD::string &err) :
	insufficient_resources(err) {}
  out_of_memory(const PGSTD::string &err, const PGSTD::string &Q) :
	insufficient_resources(err,Q) {}
};

class PQXX_LIBEXPORT too_many_connections : public broken_connection
{
public:
  explicit too_many_connections(const PGSTD::string &err) :
	broken_connection(err) {}
};

/**
 * @}
 */

}

#include "pqxx/compiler-internal-post.hxx"
