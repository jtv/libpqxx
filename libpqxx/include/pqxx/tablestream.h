/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablestream.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::TableStream class.
 *   pqxx::TableStream provides optimized batch access to a database table
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TABLESTREAM_H
#define PQXX_TABLESTREAM_H

#include <string>

#include "pqxx/compiler.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{

// TODO: Async access to help hide network latencies
// TODO: Binary COPY

class Transaction;


/// Base class for streaming data to/from database tables.
/** A Tablestream enables optimized batch read or write access to a database 
 * table using PostgreSQL's COPY TO STDOUT and COPY FROM STDIN commands,
 * respectively.  These capabilities are implemented by its subclasses 
 * TableReader and TableWriter.
 * A Tablestream exists in the context of a Transaction, and no other streams
 * or queries may be applied to that Transaction as long as the stream remains
 * open.
 */
class PQXX_LIBEXPORT TableStream
{
public:
  TableStream(Transaction &Trans, 
	      PGSTD::string Name, 
	      PGSTD::string Null=PGSTD::string());			//[t6]
  virtual ~TableStream() =0;						//[t6]

  PGSTD::string Name() const { return m_Name; }				//[t10]

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

