/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablewriter.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablewriter class.
 *   pqxx::tablewriter enables optimized batch updates to a database table
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/tablereader.h"
#include "pqxx/tablewriter.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::tablewriter::tablewriter(transaction_base &T, const string &WName) :
  tablestream(T, WName)
{
  T.BeginCopyWrite(WName);
}


pqxx::tablewriter::~tablewriter()
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


pqxx::tablewriter &pqxx::tablewriter::operator<<(pqxx::tablereader &R)
{
  string Line;
  while (R.GetRawLine(Line))
    WriteRawLine(Line);

  return *this;
}


void pqxx::tablewriter::WriteRawLine(const string &Line)
{
  Trans().WriteCopyLine(Line);
}


