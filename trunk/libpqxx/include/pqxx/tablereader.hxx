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
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include <string>

#include "pqxx/result"
#include "pqxx/tablestream"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{

/// Efficiently pull data directly out of a table.
/** A tablereader provides efficient read access to a database table.  This is
 * not as flexible as a normal query using the result class however:
 *  - Can only dump tables, not views or arbitrary queries
 *  - Has no knowledge of metadata
 *  - Is unable to reorder, rename, omit or enrich fields
 *  - Does not support filtering of records
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
      const PGSTD::string &RName,
      const PGSTD::string &Null=PGSTD::string());			//[t6]
  ~tablereader();							//[t6]

  template<typename TUPLE> tablereader &operator>>(TUPLE &);		//[t8]

  operator bool() const throw () { return !m_Done; }			//[t6]
  bool operator!() const throw () { return m_Done; }			//[t6]

  /// Read a line of raw, unparsed table data
  /** Returns whether a line could be read.
   * @param Line is set to the raw data line read from the table.
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
  void reader_close();
  static char unescapechar(char) throw ();
  static bool is_octalchar(char o) throw () { return (o>='0') && (o<='7'); }
  bool m_Done;
};


}

// TODO: Find meaningful definition of input iterator


template<typename TUPLE> 
inline void pqxx::tablereader::tokenize(PGSTD::string Line, 
                                        TUPLE &T) const
{
  if (Line.empty()) return;
  PGSTD::back_insert_iterator<TUPLE> ins = PGSTD::back_inserter(T);

  // Make sure the line is zero-terminated
  if (Line[Line.size()-1] != '\n') Line.push_back('\n');

  // Filter and tokenize line, inserting tokens at end of T
  PGSTD::string::size_type token = 0;
  for (PGSTD::string::size_type i=0; i < Line.size(); ++i)
  {
    switch (Line[i])
    {
    case '\t': // End of token
    case '\n': // End of row
      *ins++ = PGSTD::string(Line, token, i-token);
      token = i+1;
      break;

    case '\\':
      // Remove the backslash and unescape whatever comes after it 
      if ((i+1) >= Line.size()) 
      {
	throw PGSTD::runtime_error("Row ends in backslash");
      }
      else
      {
	const char n = Line[i+1];
	if (n == 'N')			// NULL value
	{
	  Line.replace(i, 2, NullStr());
	  i += NullStr().size()-1;
	}
	else if (is_octalchar(n))	// Octal character
	{
	  if ((i+3) >= Line.size())
	    throw PGSTD::runtime_error("Row ends in middle of octal value");
	  const char n1 = Line[i+2], n2 = Line[i+3];
	  if (!is_octalchar(n1) || !is_octalchar(n2))
	    throw PGSTD::runtime_error("Invalid octal value");
	  const char c = (((n-'0')<<6) | ((n1-'0')<<3) | (n2-'0'));
	  Line.replace(i, 4, 1, c);
	}
	else				// Simple escaped character
	{
	  Line.replace(i, 2, 1, unescapechar(i));
	}
      }
      break;
    }
  }
}


template<typename TUPLE> 
inline pqxx::tablereader &pqxx::tablereader::operator>>(TUPLE &T)
{
  PGSTD::string Line;
  if (get_raw_line(Line)) tokenize(Line, T);
  return *this;
}


