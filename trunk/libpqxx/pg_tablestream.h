/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_tablestream.h
 *
 *   DESCRIPTION
 *      definition of the Pg::TableStream class.
 *   Pg::TableStream provides optimized batch access to a database table
 *
 * Copyright (c) 2001, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_TABLESTREAM_H
#define PG_TABLESTREAM_H

#include <string>

#include "pg_compiler.h"


namespace Pg
{

// A Tablestream enables optimized batch read or write access to a database 
// table using PostgreSQL's COPY TO STDOUT and COPY FROM STDIN commands,
// respectively.  These capabilities are implemented by its subclasses 
// TableReader and TableWriter.
// A Tablestream exists in the context of a Transaction, and no other streams
// or queries may be applied to that Transaction as long as the stream remains
// open.

// TODO: Async access to help hide network latencies
// TODO: Binary COPY
// TODO: Range versions of TUPLE templates

class Transaction;

class TableStream
{
public:
  TableStream(Transaction &Trans, 
	      PGSTD::string Name, 
	      PGSTD::string Null=PGSTD::string());
  virtual ~TableStream() =0;

  PGSTD::string Name() const { return m_Name; }

protected:
  Transaction &Trans() const { return m_Trans; }
  PGSTD::string NullStr() const { return m_Null; }

private:
  Transaction &m_Trans;
  PGSTD::string m_Name;
  PGSTD::string m_Null;

  // Not allowed:
  TableStream();
  TableStream(const TableStream &);
  TableStream &operator=(const TableStream &);
};


}

#endif

