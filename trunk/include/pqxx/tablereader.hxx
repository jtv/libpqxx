/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablereader.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::tablereader class.
 *   pqxx::tablereader enables optimized batch reads from a database table
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablereader instead.
 *
 * Copyright (c) 2001-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"

#include "pqxx/result"
#include "pqxx/tablestream"

/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{

/// Efficiently pull data directly out of a table.
/** A tablereader provides efficient read access to a database table.  This is
 * not as flexible as a normal query using the result class however:
 * <ul>
 * <li> Can only dump tables, not views or arbitrary queries
 * <li> Has no knowledge of metadata
 * <li> Is unable to reorder, rename, omit or enrich fields
 * <li> Does not support filtering of records
 * </ul>
 *
 * On the other hand, it can read rows of data and transform them into any
 * container or container-like object that supports STL back-inserters.  Since
 * the tablereader has no knowledge of the types of data expected, it treats
 * all fields as strings.
 */
class PQXX_LIBEXPORT tablereader : public tablestream
{
public:
  tablereader(transaction_base &,
      const PGSTD::string &Name,
      const PGSTD::string &Null=PGSTD::string());			//[t6]

  /// Read only the given sequence of columns
  /** @since PostgreSQL backend 7.3
   */
  template<typename ITER>
  tablereader(transaction_base &,
      const PGSTD::string &Name,
      ITER begincolumns,
      ITER endcolumns,
      const PGSTD::string &Null=PGSTD::string());			//[t80]

  ~tablereader() throw ();						//[t6]

  template<typename TUPLE> tablereader &operator>>(TUPLE &);		//[t8]

  operator bool() const throw () { return !m_Done; }			//[t6]
  bool operator!() const throw () { return m_Done; }			//[t6]

  /// Read a line of raw, unparsed table data
  /**
   * @param Line Variable to hold the raw data line read from the table.
   * @return Whether a line could be read
   */
  bool get_raw_line(PGSTD::string &Line);				//[t8]

  template<typename TUPLE>
  void tokenize(PGSTD::string, TUPLE &) const;				//[t8]

  /// Finish stream action, check for errors, and detach from transaction
  /** It is recommended that you call this function before the tablestream's
   * destructor is run.  This function will check any final errors which may not
   * become apparent until the transaction is committed otherwise.
   *
   * As an added benefit, this will free up the transaction while the
   * tablestream object itself still exists.
   */
  virtual void complete();						//[t8]

#ifdef PQXX_DEPRECATED_HEADERS
  /// @deprecated Use get_raw_line() instead
  bool GetRawLine(PGSTD::string &L) { return get_raw_line(L); }
  /// @deprecated Use tokenize<>() instead
  template<typename TUPLE> void Tokenize(PGSTD::string L, TUPLE &T) const
  	{ tokenize(L, T); }
#endif

private:
  void setup(transaction_base &T,
      const PGSTD::string &RName,
      const PGSTD::string &Columns=PGSTD::string());
  void PQXX_PRIVATE reader_close();
  PGSTD::string extract_field(const PGSTD::string &,
      PGSTD::string::size_type &) const;

  bool m_Done;
};


// TODO: Find meaningful definition of input iterator


template<typename ITER> inline
tablereader::tablereader(transaction_base &T,
    const PGSTD::string &Name,
    ITER begincolumns,
    ITER endcolumns,
    const PGSTD::string &Null) :
  namedclass(Name, "tablereader"),
  tablestream(T, Null),
  m_Done(true)
{
  setup(T, Name, columnlist(begincolumns, endcolumns));
}


template<typename TUPLE>
inline void tablereader::tokenize(PGSTD::string Line, TUPLE &T) const
{
  PGSTD::back_insert_iterator<TUPLE> ins = PGSTD::back_inserter(T);

  // Filter and tokenize line, inserting tokens at end of T
  PGSTD::string::size_type here=0;
  while (here < Line.size()) *ins++ = extract_field(Line, here);
}


template<typename TUPLE>
inline tablereader &pqxx::tablereader::operator>>(TUPLE &T)
{
  PGSTD::string Line;
  if (get_raw_line(Line)) tokenize(Line, T);
  return *this;
}


} // namespace pqxx

