/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/except.h
 *
 *   DESCRIPTION
 *      definition of libpqxx exception classes
 *   pqxx::sql_error, pqxx::broken_connection, pqxx::in_doubt_error, ...
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_EXCEPT_H
#define PQXX_EXCEPT_H

#include <string>
#include <stdexcept>

#include "pqxx/util.h"


/// Exception class for lost backend connection.
/** (May be changed once I find a standard exception class for this) */
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
  sql_error(const PGSTD::string &whatarg) : 
	PGSTD::runtime_error(whatarg), m_Q() {}
  sql_error(const PGSTD::string &whatarg, const PGSTD::string &Q) :
	PGSTD::runtime_error(whatarg), m_Q(Q) { }
  virtual ~sql_error() throw () {}

  /// The query whose execution triggered the exception
  const PGSTD::string &query() const { return m_Q; }			//[t56]
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


#endif

