/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_cursor.h
 *
 *   DESCRIPTION
 *      definition of the Pg::Cursor class.
 *   Pg::Cursor represents a database cursor.
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_CURSOR_H
#define PG_CURSOR_H

#include "pg_util.h"

/* Cursor behaves as an output stream generating Result objects.  They may
 * be used to fetch rows individually or in blocks, in which case each Result
 * coming out of the stream will contain more than one Tuple.
 *
 * Postgres does not currently support modification of data through a cursor.
 */


/* A quick note on binary cursors:
 * These will require a lot of work.  First off, conversion to C++ datatypes
 * becomes more complex.  Second, some tradeoffs will need to be made between
 * dynamic (flexible) type handling and static (fast) type handling.
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace Pg
{
class Result;
class Transaction;

class Cursor
{
public:
  // Create a Cursor.  The optional name must begin with a letter and contain
  // letters and digits only.
  // Count is the stride of the cursor, ie. the number of rows fetched.  This
  // defaults to 1.
  Cursor(Transaction &, 
         const char Query[], 
	 PGSTD::string BaseName="cur",
	 Result_size_type Count=NEXT());				//[t3]

  Result_size_type SetCount(Result_size_type);				//[]

  Result Fetch(Result_size_type Count);					//[]
  void Move(Result_size_type Count);					//[t3]

  // Take care: ALL() and BACKWARD_ALL() may not do what they say if your
  // result set is larger than can be addressed.  In that case, moving or
  // fetching by ALL() or BACKWARD_ALL() will actually work in chunks of the
  // largest size that can be expressed in Result's size type (currently long).
  static Result_size_type ALL() { return Result_size_type_max; }	//[t3]
  static Result_size_type NEXT() { return 1; }				//[]
  static Result_size_type PRIOR() { return -1; }			//[]
  static Result_size_type BACKWARD_ALL() {return Result_size_type_min;}	//[]

  Cursor &operator>>(Result &);						//[t3]

  operator bool() const { return !m_Done; }				//[t3]
  bool operator!() const { return m_Done; }				//[]

  Cursor &operator+=(Result_size_type);					//[]
  Cursor &operator-=(Result_size_type);					//[]

private:
  PGSTD::string MakeFetchCmd(Result_size_type) const;

  Transaction &m_Trans;
  PGSTD::string m_Name;
  Result_size_type m_Count;
  bool m_Done;

  // Not allowed:
  Cursor(const Cursor &);
  Cursor &operator=(const Cursor &);
};

}

#endif

