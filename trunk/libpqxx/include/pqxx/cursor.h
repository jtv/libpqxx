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
	 PGSTD::string BaseName="cur",
	 Result_size_type Count=NEXT());				//[t3]

  /// Set new stride, ie. the number of rows to fetch at a time.
  Result_size_type SetCount(Result_size_type);				//[t19]

  /// Fetch Count rows of data.
  /** The number of rows fetched will not exceed Count, but it may be lower.
   */
  Result Fetch(Result_size_type Count);					//[t19]

  /// Move forward by Count rows (negative for backwards) through the data set.
  void Move(Result_size_type Count);					//[t3]

  /// Constant: "next fetch/move should span as many rows as possible."
  /** If the number of rows ahead exceeds the largest number your machine can
   * comfortably conceive, this may not actually be all remaining rows in the 
   * result set.
   */
  static Result_size_type ALL() { return Result_size_type_max; }	//[t3]

  /// Constant: "next fetch/move should cover just the next row."
  static Result_size_type NEXT() { return 1; }				//[t19]

  /// Constant: "next fetch/move should go back one row."
  static Result_size_type PRIOR() { return -1; }			//[t19]

  /// Constant: "next fetch/move goes backwards, spanning as many rows as 
  /// possible.
  /** If the number of rows behind the cursor exceeds the largest number your
   * machine can comfortably conceive, this may not bring you all the way back
   * to the beginning.
   */
  static Result_size_type BACKWARD_ALL() 				//[t19]
  	{ return Result_size_type_min + 1; }

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
  operator bool() const { return !m_Done; }				//[t3]
  /// Are we done?
  bool operator!() const { return m_Done; }				//[t3]

  /// Move N rows forward.
  Cursor &operator+=(Result_size_type N) { Move(N); return *this;}	//[t19]
  /// Move N rows backward.
  Cursor &operator-=(Result_size_type N) { Move(-N); return *this;}	//[t19]

private:
  static PGSTD::string OffsetString(Result_size_type);
  PGSTD::string MakeFetchCmd(Result_size_type) const;

  TransactionItf &m_Trans;
  PGSTD::string m_Name;
  Result_size_type m_Count;
  bool m_Done;

  // Not allowed:
  Cursor(const Cursor &);
  Cursor &operator=(const Cursor &);
};

}

#endif

