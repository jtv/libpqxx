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
 * Copyright (c) 2003-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include <stdexcept>

#include "pqxx/util"


namespace pqxx
{

/**
 * @addtogroup exception Exception classes
 */
//@{

/// Exception class for lost backend connection.
class PQXX_LIBEXPORT broken_connection : public PGSTD::runtime_error
{
public:
  broken_connection() : PGSTD::runtime_error("Connection to back end failed") {}
  explicit broken_connection(const PGSTD::string &whatarg) :
    PGSTD::runtime_error(whatarg) {}
};


/// Exception class for failed queries.
/** Carries a copy of the failed query in addition to a regular error message */
class PQXX_LIBEXPORT sql_error : public PGSTD::runtime_error
{
  PGSTD::string m_Q;

public:
  sql_error() : PGSTD::runtime_error("Failed query"), m_Q() {}
  explicit sql_error(const PGSTD::string &whatarg) :
	PGSTD::runtime_error(whatarg), m_Q() {}
  sql_error(const PGSTD::string &whatarg, const PGSTD::string &Q) :
	PGSTD::runtime_error(whatarg), m_Q(Q) {}
  virtual ~sql_error() throw () {}

  /// The query whose execution triggered the exception
  const PGSTD::string &query() const throw () { return m_Q; }		//[t56]
};


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
  explicit in_doubt_error(const PGSTD::string &whatarg) :
	PGSTD::runtime_error(whatarg) {}
};

//@}

}

