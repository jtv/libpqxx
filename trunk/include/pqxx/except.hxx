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
 * Copyright (c) 2003-2008, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_EXCEPT
#define PQXX_H_EXCEPT

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

/// Mixin base class to identify libpqxx-specific exception types
/**
 * If you wish to catch all exception types specific to libpqxx for some reason,
 * catch this type.  All of libpqxx's exception classes are derived from it
 * through multiple-inheritance (they also fit into the standard library's
 * exception hierarchy in more fitting places).
 *
 * This class is not derived from std::exception, since that could easily lead
 * to exception classes with multiple std::exception base-class objects.  As
 * Bart Samwel points out, "catch" is subject to some nasty fineprint in such
 * cases.
 */
class PQXX_LIBEXPORT PQXX_NOVTABLE pqxx_exception
{
public:
  /// Support run-time polymorphism, and keep this class abstract
  virtual ~pqxx_exception() throw () =0;

  /// Return std::exception base-class object
  /** Use this to get at the exception's what() function, or to downcast to a
   * more specific type using dynamic_cast.
   *
   * Casting directly from pqxx_exception to a specific exception type is not
   * likely to work since pqxx_exception is not (and could not safely be)
   * derived from std::exception.
   *
   * For example, to test dynamically whether an exception is an sql_error:
   *
   * @code
   * try
   * {
   *   // ...
   * }
   * catch (const pqxx::pqxx_exception &e)
   * {
   *   std::cerr << e.base().what() << std::endl;
   *   const pqxx::sql_error *s=dynamic_cast<const pqxx::sql_error*>(&e.base());
   *   if (s) std::cerr << "Query was: " << s->query() << std::endl;
   * }
   * @endcode
   */
  virtual const PQXX_CONST PGSTD::exception &base() const throw () =0;	//[t0]
};


/// Run-time failure encountered by libpqxx, similar to std::runtime_error
class PQXX_LIBEXPORT failure :
  public pqxx_exception, public PGSTD::runtime_error
{
  virtual const PGSTD::exception &base() const throw () { return *this; }
public:
  explicit failure(const PGSTD::string &);
};


/// Exception class for lost or failed backend connection.
/**
 * @warning When this happens on Unix-like systems, you may also get a SIGPIPE
 * signal.  That signal aborts the program by default, so if you wish to be able
 * to continue after a connection breaks, be sure to disarm this signal.
 *
 * If you're working on a Unix-like system, see the manual page for
 * @c signal (2) on how to deal with SIGPIPE.  The easiest way to make this
 * signal harmless is to make your program ignore it:
 *
 * @code
 * #include <signal.h>
 *
 * int main()
 * {
 *   signal(SIGPIPE, SIG_IGN);
 *   // ...
 * @endcode
 */
class PQXX_LIBEXPORT broken_connection : public failure
{
public:
  broken_connection();
  explicit broken_connection(const PGSTD::string &);
};


/// Exception class for failed queries.
/** Carries a copy of the failed query in addition to a regular error message */
class PQXX_LIBEXPORT sql_error : public failure
{
  PGSTD::string m_Q;

public:
  sql_error();
  explicit sql_error(const PGSTD::string &);
  sql_error(const PGSTD::string &, const PGSTD::string &Q);
  virtual ~sql_error() throw ();

  /// The query whose execution triggered the exception
  const PGSTD::string & PQXX_PURE query() const throw ();		//[t56]
};


// TODO: should this be called statement_completion_unknown!?
/// "Help, I don't know whether transaction was committed successfully!"
/** Exception that might be thrown in rare cases where the connection to the
 * database is lost while finishing a database transaction, and there's no way
 * of telling whether it was actually executed by the backend.  In this case
 * the database is left in an indeterminate (but consistent) state, and only
 * manual inspection will tell which is the case.
 */
class PQXX_LIBEXPORT in_doubt_error : public failure
{
public:
  explicit in_doubt_error(const PGSTD::string &);
};


/// Internal error in libpqxx library
class PQXX_LIBEXPORT internal_error :
  public pqxx_exception, public PGSTD::logic_error
{
  virtual const PGSTD::exception &base() const throw () { return *this; }
public:
  explicit internal_error(const PGSTD::string &);
};


/// Error in usage of libpqxx library, similar to std::logic_error
class PQXX_LIBEXPORT usage_error :
  public pqxx_exception, public PGSTD::logic_error
{
  virtual const PGSTD::exception &base() const throw () { return *this; }
public:
  explicit usage_error(const PGSTD::string &);
};


/// Invalid argument passed to libpqxx, similar to std::invalid_argument
class PQXX_LIBEXPORT argument_error :
  public pqxx_exception, public PGSTD::invalid_argument
{
  virtual const PGSTD::exception &base() const throw () { return *this; }
public:
  explicit argument_error(const PGSTD::string &);
};


class PQXX_LIBEXPORT conversion_error :
  public pqxx_exception, public PGSTD::domain_error
{
  virtual const PGSTD::exception &base() const throw () { return *this; }
public:
  explicit conversion_error(const PGSTD::string &);
};


/// Something is out of range, similar to std::out_of_range
class PQXX_LIBEXPORT range_error :
  public pqxx_exception, public PGSTD::out_of_range
{
  virtual const PGSTD::exception &base() const throw () { return *this; }
public:
  explicit range_error(const PGSTD::string &);
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

class PQXX_LIBEXPORT restrict_violation :
  public integrity_constraint_violation
{
public:
  explicit restrict_violation(const PGSTD::string &err) :
	integrity_constraint_violation(err) {}
  restrict_violation(const PGSTD::string &err,
	const PGSTD::string &Q) :
	integrity_constraint_violation(err, Q) {}
};

class PQXX_LIBEXPORT not_null_violation :
  public integrity_constraint_violation
{
public:
  explicit not_null_violation(const PGSTD::string &err) :
	integrity_constraint_violation(err) {}
  not_null_violation(const PGSTD::string &err,
	const PGSTD::string &Q) :
	integrity_constraint_violation(err, Q) {}
};

class PQXX_LIBEXPORT foreign_key_violation :
  public integrity_constraint_violation
{
public:
  explicit foreign_key_violation(const PGSTD::string &err) :
	integrity_constraint_violation(err) {}
  foreign_key_violation(const PGSTD::string &err,
	const PGSTD::string &Q) :
	integrity_constraint_violation(err, Q) {}
};

class PQXX_LIBEXPORT unique_violation :
  public integrity_constraint_violation
{
public:
  explicit unique_violation(const PGSTD::string &err) :
	integrity_constraint_violation(err) {}
  unique_violation(const PGSTD::string &err,
	const PGSTD::string &Q) :
	integrity_constraint_violation(err, Q) {}
};

class PQXX_LIBEXPORT check_violation :
  public integrity_constraint_violation
{
public:
  explicit check_violation(const PGSTD::string &err) :
	integrity_constraint_violation(err) {}
  check_violation(const PGSTD::string &err,
	const PGSTD::string &Q) :
	integrity_constraint_violation(err, Q) {}
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
  /// Approximate position in string where error occurred, or -1 if unknown.
  const int error_position;

  explicit syntax_error(const PGSTD::string &err, int pos=-1) :
    sql_error(err), error_position(pos) {}
  syntax_error(const PGSTD::string &err, const PGSTD::string &Q, int pos=-1) :
	sql_error(err,Q), error_position(pos) {}
};

class PQXX_LIBEXPORT undefined_column : public syntax_error
{
public:
  explicit undefined_column(const PGSTD::string &err) : syntax_error(err) {}
  undefined_column(const PGSTD::string &err, const PGSTD::string &Q) :
    syntax_error(err, Q) {}
};

class PQXX_LIBEXPORT undefined_function : public syntax_error
{
public:
  explicit undefined_function(const PGSTD::string &err) : syntax_error(err) {}
  undefined_function(const PGSTD::string &err, const PGSTD::string &Q) :
    syntax_error(err, Q) {}
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

/// PL/pgSQL error
/** Exceptions derived from this class are errors from PL/pgSQL procedures.
 */
class PQXX_LIBEXPORT plpgsql_error : public sql_error
{
public:
  explicit plpgsql_error(const PGSTD::string &err) :
    sql_error(err) {}
  plpgsql_error(const PGSTD::string &err, const PGSTD::string &Q) :
    sql_error(err, Q) {}
};

/// Exception raised in PL/pgSQL procedure
class PQXX_LIBEXPORT plpgsql_raise : public plpgsql_error
{
public:
  explicit plpgsql_raise(const PGSTD::string &err) :
    plpgsql_error(err) {}
  plpgsql_raise(const PGSTD::string &err, const PGSTD::string &Q) :
    plpgsql_error(err, Q) {}
};

class PQXX_LIBEXPORT plpgsql_no_data_found : public plpgsql_error
{
public:
  explicit plpgsql_no_data_found(const PGSTD::string &err) :
    plpgsql_error(err) {}
  plpgsql_no_data_found(const PGSTD::string &err, const PGSTD::string &Q) :
    plpgsql_error(err, Q) {}
};

class PQXX_LIBEXPORT plpgsql_too_many_rows : public plpgsql_error
{
public:
  explicit plpgsql_too_many_rows(const PGSTD::string &err) :
    plpgsql_error(err) {}
  plpgsql_too_many_rows(const PGSTD::string &err, const PGSTD::string &Q) :
    plpgsql_error(err, Q) {}
};

/**
 * @}
 */

}

#include "pqxx/compiler-internal-post.hxx"

#endif

