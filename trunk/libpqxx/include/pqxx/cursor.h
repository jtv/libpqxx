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

#include "pqxx/result.h"
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
class TransactionItf;

/// SQL cursor class.
/** Cursor behaves as an output stream generating Result objects.  They may
 * be used to fetch rows individually or in blocks, in which case each Result
 * coming out of the stream may contain more than one Tuple.
 *
 * Postgres does not currently support modification of data through a cursor.
 * Also, not all queries support cursors that go backwards.  Unfortunately there
 * is no documentation on which queries do and which queries don't so you may
 * have to experiment before using cursors for anything but plain forward-only
 * result set iteration.
 */

class PQXX_LIBEXPORT Cursor
{
public:
  typedef Result::size_type size_type;

  /// Constructor.  Creates a cursor.
  /** 
   * @param T is the transaction that this cursor lives in.
   * @param Query defines a data set that the cursor should traverse.
   * @param BaseName optional name for the cursor, must begin with a letter 
   * and contain letters and digits only.  
   * @param Count the stride of the cursor, ie. the number of rows fetched at a
   * time.  This defaults to 1.
   */
  Cursor(TransactionItf &T,
         const char Query[], 
	 const PGSTD::string &BaseName="cur",
	 size_type Count=NEXT());					//[t3]

  /// Set new stride, ie. the number of rows to fetch at a time.
  size_type SetCount(size_type);					//[t19]

  /// Fetch Count rows of data.
  /** The number of rows fetched will not exceed Count, but it may be lower.
   */
  Result Fetch(size_type Count);					//[t19]

  /// Move forward by Count rows (negative for backwards) through the data set.
  /** Returns the number of rows skipped, as reported by PostgreSQL.  Be warned 
   * that this number may be misleading in some border cases; please do your 
   * own testing and report what you find to me!
   */
  size_type Move(size_type Count);					//[]

  /// Constant: "next fetch/move should span as many rows as possible."
  /** If the number of rows ahead exceeds the largest number your machine can
   * comfortably conceive, this may not actually be all remaining rows in the 
   * result set.
   */
  static size_type ALL() throw ()					//[t3]
  	{ return PGSTD::numeric_limits<Result::size_type>::max(); }

  /// Constant: "next fetch/move should cover just the next row."
  static size_type NEXT() throw () { return 1; }			//[t19]

  /// Constant: "next fetch/move should go back one row."
  static size_type PRIOR() throw () { return -1; }		//[t19]

  /// Constant: "next fetch/move goes backwards, spanning as many rows as 
  /// possible.
  /** If the number of rows behind the cursor exceeds the largest number your
   * machine can comfortably conceive, this may not bring you all the way back
   * to the beginning.
   */
  static size_type BACKWARD_ALL() throw ()				//[t19]
  	{ return PGSTD::numeric_limits<Result::size_type>::min() + 1; }

  /// Fetch rows.
  /** The number of rows retrieved will be no larger than (but may be lower
   * than) the rowcount set using the SetCount() function, or passed to the
   * constructor as the Count argument.  The default is 1.
   * This operator lends itself to "while (Cur >> R) { ... }"-style use; the
   * Cursor's conversion to bool tests whether it has arrived at the end of its
   * data set.
   */
  Cursor &operator>>(Result &);						//[t3]

  /// May there be more rows coming?
  operator bool() const throw () { return !m_Done; }			//[t3]
  /// Are we done?
  bool operator!() const throw () { return m_Done; }			//[t3]

  /// Move N rows forward.
  Cursor &operator+=(size_type N) { Move(N); return *this;}		//[t19]
  /// Move N rows backward.
  Cursor &operator-=(size_type N) { Move(-N); return *this;}		//[t19]

private:
  static PGSTD::string OffsetString(size_type);
  PGSTD::string MakeFetchCmd(size_type) const;

  TransactionItf &m_Trans;
  PGSTD::string m_Name;
  size_type m_Count;
  bool m_Done;

  // Not allowed:
  Cursor(const Cursor &);
  Cursor &operator=(const Cursor &);
};

}

#endif

