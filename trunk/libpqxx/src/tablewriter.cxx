/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablewriter.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::TableWriter class.
 *   pqxx::TableWriter enables optimized batch updates to a database table
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/tablereader.h"
#include "pqxx/tablewriter.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::TableWriter::TableWriter(Transaction &T, string WName) :
  TableStream(T, WName)
{
  T.BeginCopyWrite(WName);
}


pqxx::TableWriter::~TableWriter()
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


pqxx::TableWriter &pqxx::TableWriter::operator<<(pqxx::TableReader &R)
{
  string Line;
  while (R.GetRawLine(Line))
    WriteRawLine(Line);

  return *this;
}


void pqxx::TableWriter::WriteRawLine(string Line)
{
  Trans().WriteCopyLine(Line);
}


