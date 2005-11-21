/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablestream.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::tablestream class.
 *   pqxx::tablestream provides optimized batch access to a database table
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablestream instead.
 *
 * Copyright (c) 2001-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include "pqxx/transaction_base"

/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */


namespace pqxx
{
class transaction_base;


/// Base class for streaming data to/from database tables.
/** A Tablestream enables optimized batch read or write access to a database
 * table using PostgreSQL's COPY TO STDOUT and COPY FROM STDIN commands,
 * respectively.  These capabilities are implemented by its subclasses
 * tablereader and tablewriter.
 *
 * A Tablestream exists in the context of a transaction, and no other streams
 * or queries may be applied to that transaction as long as the stream remains
 * open.
 */
class PQXX_LIBEXPORT tablestream : public internal::transactionfocus
{
public:
  explicit tablestream(transaction_base &Trans,
	      const PGSTD::string &Null=PGSTD::string());		//[t6]

  virtual ~tablestream() throw () =0;					//[t6]

  /// Finish stream action, check for errors, and detach from transaction
  /** It is recommended that you call this function before the tablestream's
   * destructor is run.  This function will check any final errors which may not
   * become apparent until the transaction is committed otherwise.
   *
   * As an added benefit, this will free up the transaction while the
   * tablestream object itself still exists.
   */
  virtual void complete() =0;

#ifdef PQXX_DEPRECATED_HEADERS
  /// @deprecated Use name() instead
  PGSTD::string Name() const { return name(); }
#endif

protected:
  const PGSTD::string &NullStr() const { return m_Null; }
  bool is_finished() const throw () { return m_Finished; }
  void base_close();

  /// Construct a comma-separated column list from given sequence
  template<typename ITER>
  static PGSTD::string columnlist(ITER colbegin, ITER colend);

private:
  PGSTD::string m_Null;
  bool m_Finished;

  // Not allowed:
  tablestream();
  tablestream(const tablestream &);
  tablestream &operator=(const tablestream &);
};


template<typename ITER> inline
PGSTD::string tablestream::columnlist(ITER colbegin, ITER colend)
{
  return separated_list(",", colbegin, colend);
}
} // namespace pqxx

