/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_tablewriter.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::TableWriter class.
 *   Pg::TableWriter enables optimized batch updates to a database table
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pg_tablereader.h"
#include "pg_tablewriter.h"
#include "pg_transaction.h"

using namespace PGSTD;


Pg::TableWriter::TableWriter(Transaction &T, string WName) :
  TableStream(T, WName)
{
  T.BeginCopyWrite(WName);
}


Pg::TableWriter::~TableWriter()
{
  try
  {
    Trans().WriteCopyLine("\\.");
  }
  catch (const exception &e)
  {
    Trans().ProcessNotice(e.what());
  }
}


Pg::TableWriter &Pg::TableWriter::operator<<(Pg::TableReader &R)
{
  string Line;
  while (R.GetRawLine(Line))
    WriteRawLine(Line);

  return *this;
}


void Pg::TableWriter::WriteRawLine(string Line)
{
  Trans().WriteCopyLine(Line);
}


