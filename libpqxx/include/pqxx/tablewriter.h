/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablewriter.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::TableWriter class.
 *   pqxx::TableWriter enables optimized batch updates to a database table
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TABLEWRITER_H
#define PQXX_TABLEWRITER_H

#include "config.h"

#include <numeric>
#include <string>

#include "pqxx/tablestream.h"


/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace pqxx
{
class TableReader;	// See pqxx/tablereader.h

/** A TableWriter provides a Spartan but efficient way of writing data tuples
 * into a table.  It provides a plethora of STL-like insertion methods: it has
 * insert() methods, push_back(), an overloaded insertion operator (<<), and it
 * supports inserters created by std::back_inserter().  All of these are
 * templatized so you can use any container type or iterator range to feed 
 * tuples into the table.
 * Note that in each case, a container or range represents the fields of a 
 * single tuple--not a collection of tuples.
 */
class PQXX_LIBEXPORT TableWriter : public TableStream
{
public:
  typedef unsigned size_type;

  TableWriter(Transaction &Trans, PGSTD::string Name);			//[t5]
  ~TableWriter();							//[t5]

  template<typename IT> void insert(IT Begin, IT End);			//[t5]
  template<typename TUPLE> void insert(const TUPLE &);			//[t5]
  template<typename IT> void push_back(IT Begin, IT End);		//[t10]
  template<typename TUPLE> void push_back(const TUPLE &);		//[t10]

  void reserve(size_type) {}						//[]

  template<typename TUPLE> TableWriter &operator<<(const TUPLE &);	//[t5]

  // Copy table from one database to another
  TableWriter &operator<<(TableReader &);				//[t6]

  /// "Untokenize" a tuple of data to a string in DBMS-specific format.  This
  /// is not portable between databases.
  template<typename IT> PGSTD::string ezinekoT(IT Begin, IT End) const;	//[t10]
  template<typename TUPLE> PGSTD::string ezinekoT(const TUPLE &) const;	//[t10]

private:
  void WriteRawLine(PGSTD::string);

  class FieldConverter
  {
  public:
    FieldConverter(const PGSTD::string &N) : Null(N) {}
    template<typename T> PGSTD::string operator()(const PGSTD::string &,
		                                  T) const;
    PGSTD::string operator()(const PGSTD::string &, const char *) const;
  private:
    static PGSTD::string PGNull() { return "\\N"; }
    static void Escape(PGSTD::string &);
    PGSTD::string Null;
  };
};

}


namespace PGSTD
{
// Specialized back_insert_iterator for TableWriter, doesn't require a 
// value_type to be defined.  Accepts any container type instead.
template<> class back_insert_iterator<pqxx::TableWriter> : 		//[t9]
	public iterator<output_iterator_tag, void,void,void,void>
{
public:
  explicit back_insert_iterator(pqxx::TableWriter &W) : m_Writer(W) {}

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
  pqxx::TableWriter &m_Writer;
};

}



void pqxx::TableWriter::FieldConverter::Escape(PGSTD::string &S)
{
  const char Special[] = "\n\t\\";

  // TODO: Is this legal if that j+2 happens to point just past the end of S?
  for (PGSTD::string::size_type j = S.find_first_of(Special);
       j != PGSTD::string::npos;
       j = S.find_first_of(Special, j+2))
    S.insert(j, 1, '\\');
}


template<typename T> inline PGSTD::string
pqxx::TableWriter::FieldConverter::operator()(const PGSTD::string &S,
		                              T i) const
{
  PGSTD::string Field = ToString(i);
  return S + ((Field == Null) ? PGNull() : Field);
}


template<> inline PGSTD::string 
pqxx::TableWriter::FieldConverter::operator()(const PGSTD::string &S,
		                              PGSTD::string i) const
{
  if (i == Null) i = PGNull();
  else Escape(i);
  return S + i + '\t';
}


inline PGSTD::string
pqxx::TableWriter::FieldConverter::operator()(const PGSTD::string &S,
		                              const char *i) const
{
  return operator()(S, PGSTD::string(i));
}


template<typename IT> 
inline PGSTD::string pqxx::TableWriter::ezinekoT(IT Begin, IT End) const
{
  PGSTD::string Line = PGSTD::accumulate(Begin, 
		                         End, 
					 PGSTD::string(), 
					 FieldConverter(NullStr()));

  // Above algorithm generates one separating tab too many.  Take it back.
  if (!Line.empty()) Line.erase(Line.size()-1);

  return Line;
}


template<typename TUPLE> 
inline PGSTD::string pqxx::TableWriter::ezinekoT(const TUPLE &T) const
{
  return ezinekoT(T.begin(), T.end());
}


template<typename IT> inline void pqxx::TableWriter::insert(IT Begin, IT End)
{
  WriteRawLine(ezinekoT(Begin, End));
}


template<typename TUPLE> inline void pqxx::TableWriter::insert(const TUPLE &T)
{
  insert(T.begin(), T.end());
}

template<typename IT> 
inline void pqxx::TableWriter::push_back(IT Begin, IT End)
{
  insert(Begin, End);
}

template<typename TUPLE> 
inline void pqxx::TableWriter::push_back(const TUPLE &T)
{
  insert(T.begin(), T.end());
}

template<typename TUPLE> 
inline pqxx::TableWriter &pqxx::TableWriter::operator<<(const TUPLE &T)
{
  insert(T);
  return *this;
}


#endif

