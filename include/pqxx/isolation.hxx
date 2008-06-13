/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/isolation.hxx
 *
 *   DESCRIPTION
 *      definitions of transaction isolation levels
 *   Policies and traits describing SQL transaction isolation levels
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/isolation instead.
 *
 * Copyright (c) 2003-2008, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_ISOLATION
#define PQXX_H_ISOLATION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/util"

namespace pqxx
{

/// Transaction isolation levels; PostgreSQL doesn't implement all SQL levels
/** The only levels implemented in postgres are read_committed and serializable;
 * SQL also defines read_uncommitted and repeatable_read.  Unless you're bent on
 * using nasty tricks to communicate between ongoing transactions and such, you
 * won't really need isolation levels for anything except performance
 * optimization.  In that case, you can safely emulate read_uncommitted by using
 * read_committed and repeatable_read by using serializable.  In general,
 * serializable is the safest choice.
 */
enum isolation_level
{
  // read_uncommitted,
  read_committed,
  // repeatable_read,
  serializable
};

/// Traits class to describe an isolation level; primarly for libpqxx's own use
template<isolation_level LEVEL> struct isolation_traits
{
  static isolation_level level() throw () { return LEVEL; }
  static const char *name() throw ();
};


template<> inline const char *isolation_traits<read_committed>::name() throw ()
	{ return "READ COMMITTED"; }
template<> inline const char *isolation_traits<serializable>::name() throw ()
	{ return "SERIALIZABLE"; }

}


#include "pqxx/compiler-internal-post.hxx"

#endif

