/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_tablewriter.h
 *
 *   DESCRIPTION
 *      definition of the Pg::TableWriter class.
 *   Pg::TableWriter enables optimized batch updates to a database table
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_TABLEWRITER_H
#define PG_TABLEWRITER_H

#include <string>

#include "pg_tablestream.h"


/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace Pg
{
class TableReader;	// See pg_tablereader.h

class TableWriter : public TableStream
{
public:
  TableWriter(Transaction &Trans, PGSTD::string Name);			//[t5]
  ~TableWriter();							//[t5]

  template<typename TUPLE> TableWriter &operator<<(const TUPLE &);
  template<typename TUPLE> void insert(const TUPLE &);			//[t5]
  template<typename TUPLE> void push_back(const TUPLE &T) {insert(T);}	//[t10]

  // Copy table from one database to another
  TableWriter &operator<<(TableReader &);				//[t6]

  template<typename TUPLE> PGSTD::string ezinekoT(const TUPLE &) const;	//[t10]

private:
  void WriteRawLine(PGSTD::string);
};

}


// Specialized back_insert_iterator for TableWriter, doesn't require a 
// value_type to be defined.  Accepts any container type instead.
template<> class PGSTD::back_insert_iterator<Pg::TableWriter> : 	//[t9]
	public PGSTD::iterator<PGSTD::output_iterator_tag, void,void,void,void>
{
public:
  explicit back_insert_iterator(Pg::TableWriter &W) : m_Writer(W) {}

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
  Pg::TableWriter &m_Writer;
};




template<typename TUPLE> 
inline PGSTD::string Pg::TableWriter::ezinekoT(const TUPLE &T) const
{
  PGSTD::string Line;

  for (typename TUPLE::const_iterator i=T.begin(); i!=T.end(); ++i)
  {
    PGSTD::string Field = ToString(*i);

    if (Field == NullStr())
    {
      Field = "\\N";
    }
    else
    {
      for (PGSTD::string::size_type j=0; j<Field.size(); ++j)
      {
        switch (Field[j])
        {
        case '\n':
        case '\t':
	case '\\':
	  Field.insert(j, 1, '\\');
	  ++j;
	  break;
        }
      }
    }

    Line += Field;
    Line += '\t';
  }

  // Above algorithm generates one separating tab too many.  Take it back.
  if (!Line.empty())
    Line.erase(Line.size()-1);

  return Line;
}


template<typename TUPLE> inline void Pg::TableWriter::insert(const TUPLE &T)
{
  WriteRawLine(ezinekoT(T));
}


template<typename TUPLE> 
inline Pg::TableWriter &Pg::TableWriter::operator<<(const TUPLE &T)
{
  insert(T);
  return *this;
}


#endif

