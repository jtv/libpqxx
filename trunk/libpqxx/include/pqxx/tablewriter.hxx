/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablewriter.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::tablewriter class.
 *   pqxx::tablewriter enables optimized batch updates to a database table
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablewriter.hxx instead.
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <numeric>
#include <string>

#include "pqxx/tablestream"


/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace pqxx
{
class tablereader;	// See pqxx/tablereader.h

/// Efficiently write data directly to a database table.
/** A tablewriter provides a Spartan but efficient way of writing data tuples
 * into a table.  It provides a plethora of STL-like insertion methods: it has
 * insert() methods, push_back(), as well as an overloaded insertion operator 
 * (<<), and it supports inserters created by std::back_inserter().  All of 
 * these are templatized so you can use any container type or iterator range to
 * feed tuples into the table.
 * Note that in each case, a container or range represents the fields of a 
 * single tuple--not a collection of tuples.
 */
class PQXX_LIBEXPORT tablewriter : public tablestream
{
public:
  typedef unsigned size_type;

  tablewriter(transaction_base &Trans, const PGSTD::string &WName);	//[t5]
  ~tablewriter();							//[t5]

  template<typename IT> void insert(IT Begin, IT End);			//[t5]
  template<typename TUPLE> void insert(const TUPLE &);			//[t5]
  template<typename IT> void push_back(IT Begin, IT End);		//[t10]
  template<typename TUPLE> void push_back(const TUPLE &);		//[t10]

  void reserve(size_type) {}						//[t9]

  template<typename TUPLE> tablewriter &operator<<(const TUPLE &);	//[t5]

  // Copy table from one database to another
  tablewriter &operator<<(tablereader &);				//[t6]

#ifdef PQXX_DEPRECATED_HEADERS
  /// @deprecated Use generate() instead
  template<typename IT> PGSTD::string ezinekoT(IT Begin, IT End) const
    	{ return generate(Begin, End); }
  /// @deprecated Use generate() instead
  template<typename TUPLE> PGSTD::string ezinekoT(const TUPLE &T) const
    	{ return generate(T); }
#endif

  /// Translate tuple of data to a string in DBMS-specific format.  This
  /// is not portable between databases.
  template<typename IT> PGSTD::string generate(IT Begin, IT End) const;	//[t10]
  template<typename TUPLE> PGSTD::string generate(const TUPLE &) const;	//[t10]


private:
  void WriteRawLine(const PGSTD::string &);

  class PQXX_LIBEXPORT fieldconverter
  {
  public:
    fieldconverter(const PGSTD::string &N) : Null(N) {}

    template<typename T> PGSTD::string operator()(const PGSTD::string &S,
		                                  T i) const
    {
      PGSTD::string Field(ToString(i));
      return S + ((Field == Null) ? PGNull() : Field);
    }

#ifdef PQXX_NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION
    template<> PGSTD::string operator()(const PGSTD::string &S,
	                                PGSTD::string i) const;
#endif

    PGSTD::string operator()(const PGSTD::string &S, const char *i) const;

  private:
    static PGSTD::string PGNull() { return "\\N"; }
    static void Escape(PGSTD::string &);
    PGSTD::string Null;
  };
};

}


namespace PGSTD
{
// Specialized back_insert_iterator for tablewriter, doesn't require a 
// value_type to be defined.  Accepts any container type instead.
template<> 
  class back_insert_iterator<pqxx::tablewriter> :			//[t9]
	public iterator<output_iterator_tag, void,void,void,void>
{
public:
  explicit back_insert_iterator(pqxx::tablewriter &W) : m_Writer(W) {}

  template<typename TUPLE> 
  back_insert_iterator &operator=(const TUPLE &T)
  {
    m_Writer.insert(T);
    return *this;
  }

  back_insert_iterator &operator++() { return *this; }
  back_insert_iterator &operator++(int) { return *this; }
  back_insert_iterator &operator*() { return *this; }

private:
  pqxx::tablewriter &m_Writer;
};

} // namespace


namespace pqxx
{

template<>
inline PGSTD::string 
tablewriter::fieldconverter::operator()(const PGSTD::string &S,
                                        PGSTD::string i) const
{
  if (i == Null) i = PGNull();
  else Escape(i);
  return S + i + '\t';
}


inline PGSTD::string
tablewriter::fieldconverter::operator()(const PGSTD::string &S, 
                                        const char *i) const
{
  return operator()(S, PGSTD::string(i));
}


inline void tablewriter::fieldconverter::Escape(PGSTD::string &S)
{
  const char Special[] = "\n\t\\";

  for (PGSTD::string::size_type j = S.find_first_of(Special);
       j != PGSTD::string::npos;
       j = S.find_first_of(Special, j+2))
    S.insert(j, 1, '\\');
}


template<typename IT> 
inline PGSTD::string tablewriter::generate(IT Begin, IT End) const
{
  PGSTD::string Line = PGSTD::accumulate(Begin, 
		                         End, 
					 PGSTD::string(), 
					 fieldconverter(NullStr()));

  // Above algorithm generates one separating tab too many.  Take it back.
  if (!Line.empty()) Line.erase(Line.size()-1);

  return Line;
}


template<typename TUPLE> 
inline PGSTD::string tablewriter::generate(const TUPLE &T) const
{
  return generate(T.begin(), T.end());
}


template<typename IT> inline void tablewriter::insert(IT Begin, IT End)
{
  WriteRawLine(generate(Begin, End));
}


template<typename TUPLE> inline void tablewriter::insert(const TUPLE &T)
{
  insert(T.begin(), T.end());
}

template<typename IT> 
inline void tablewriter::push_back(IT Begin, IT End)
{
  insert(Begin, End);
}

template<typename TUPLE> 
inline void tablewriter::push_back(const TUPLE &T)
{
  insert(T.begin(), T.end());
}

template<typename TUPLE> 
inline tablewriter &tablewriter::operator<<(const TUPLE &T)
{
  insert(T);
  return *this;
}

}


