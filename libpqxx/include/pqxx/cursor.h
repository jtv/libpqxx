/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cursor.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::Cursor class.
 *   pqxx::Cursor represents a database cursor.
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CURSOR_H
#define PQXX_CURSOR_H

#include "pqxx/util.h"

/* (A quick note on binary cursors:
 * These will require a lot of work.  First off, conversion to C++ datatypes
 * becomes more complex.  Second, some tradeoffs will need to be made between
 * dynamic (flexible) type handling and static (fast) type handling.)
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace pqxx
{
class Result;
class Transaction;

/// SQL cursor class.
/** Cursor behaves as an output stream generating Result objects.  They may
 * be used to fetch rows individually or in blocks, in which case each Result
 * coming out of the stream will contain more than one Tuple.
 *
 * Postgres does not currently support modification of data through a cursor.
 */

class PQXX_LIBEXPORT Cursor
{
public:
  /// Constructor.  Creates a cursor.
  /** 
   * @param T is the transaction that this cursor lives in.
   * @param Query defines a data set that the cursor should traverse.
   * @param BaseName optional name for the cursor, must begin with a letter 
   * and contain letters and digits only.  
   * @param Count the stride of the cursor, ie. the number of rows fetched at a
   * time.  This defaults to 1.
   */
  Cursor(Transaction &T,
         const char Query[], 
	 PGSTD::string BaseName="cur",
	 Result_size_type Count=NEXT());				//[t3]

  /// Set new stride, ie. the number of rows to fetch at a time.
  Result_size_type SetCount(Result_size_type);				//[t19]

  /// Fetch Count rows of data.
  Result Fetch(Result_size_type Count);					//[t19]

  /// Move forward by Count rows (negative for backwards) through the data set.
  void Move(Result_size_type Count);					//[t3]

  // Take care: ALL() and BACKWARD_ALL() may not do what they say if your
  // result set is larger than can be addressed.  In that case, moving or
  // fetching by ALL() or BACKWARD_ALL() will actually work in chunks of the
  // largest size that can be expressed in Result's size type (currently long).

  // TODO: BACKWARD_ALL() is kludgy.
  static Result_size_type ALL() { return Result_size_type_max; }	//[t3]
  static Result_size_type NEXT() { return 1; }				//[t19]
  static Result_size_type PRIOR() { return -1; }			//[t19]
  static Result_size_type BACKWARD_ALL() 				//[t19]
  	{ return Result_size_type_min + 1; }

  /// Fetch rows.
  Cursor &operator>>(Result &);						//[t3]

  /// May there be more rows coming?
  operator bool() const { return !m_Done; }				//[t3]
  /// Are we done?
  bool operator!() const { return m_Done; }				//[t3]

  /// Move N rows forward.
  Cursor &operator+=(Result_size_type N) { Move(N); return *this;}	//[t19]
  /// Move N rows backward.
  Cursor &operator-=(Result_size_type N) { Move(-N); return *this;}	//[t19]

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

