/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cursor.hxx
 *
 *   DESCRIPTION
 *      definition of the iterator/container-style cursor classes
 *   C++-style wrappers for SQL cursors
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/pipeline instead.
 *
 * Copyright (c) 2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include <string>

#include "pqxx/result"


namespace pqxx
{
class transaction_base;

/// Common definitions for cursor types
class PQXX_LIBEXPORT cursor_base
{
  enum { off_prior=-1, off_next=1 };
public:
  typedef result::size_type size_type;

  static size_type ALL() throw ();
  static size_type NEXT() throw () { return off_next; }
  static size_type PRIOR() throw () { return off_prior; }
  static size_type BACKWARD_ALL() throw ();

protected:
  cursor_base() : m_context(0) {}
  cursor_base(transaction_base *context) : m_context(context) {}

  int get_unique_cursor_num();

  transaction_base *m_context;
};


cursor_base::size_type cursor_base::ALL() throw ()
{
#ifdef _WIN32
  // Microsoft's compiler defines max() and min() macros!  Others may as well
  return INT_MAX;
#else
  return numeric_limits<size_type>::max();
#endif
}

cursor_base::size_type cursor_base::BACKWARD_ALL() throw ()
{
#ifdef _WIN32
  return INT_MIN + 1;
#else
  return numeric_limits<size_type>::min() + 1;
#endif
}


/// Simple "forward-only input iterator" wrapper for SQL cursor
class PQXX_LIBEXPORT const_forward_cursor : public cursor_base
{
public:
  const_forward_cursor();						//[]

  const_forward_cursor(const const_forward_cursor &);			//[]

  const_forward_cursor(transaction_base &, 				//[]
      const PGSTD::string &query,
      size_type stride=1);

  const_forward_cursor(transaction_base &, 				//[]
      const PGSTD::string &query,
      const PGSTD::string &cname,
      size_type stride=1);

  const_forward_cursor &operator++();					//[]
  const_forward_cursor operator++(int);					//[]

  result operator*() const { return m_lastdata; }			//[]
  const result &operator->() const throw () { return m_lastdata; }	//[]

  bool operator==(const const_forward_cursor &) const;			//[]
  const_forward_cursor &operator=(const const_forward_cursor &);	//[]

private:
  void declare(const PGSTD::string &);
  void fetch();

  PGSTD::string m_name;
  size_type m_stride;
  result m_lastdata;
};


} // namespace pqxx

