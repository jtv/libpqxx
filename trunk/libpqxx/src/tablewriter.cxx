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
#include "pqxx/compiler.h"

#include "pqxx/tablereader"
#include "pqxx/tablewriter"
#include "pqxx/transaction"

using namespace PGSTD;


pqxx::tablewriter::tablewriter(transaction_base &T, const string &WName) :
  tablestream(T, WName)
{
  T.BeginCopyWrite(WName);
}


pqxx::tablewriter::~tablewriter()
{
  Trans().EndCopyWrite();
}


pqxx::tablewriter &pqxx::tablewriter::operator<<(pqxx::tablereader &R)
{
  string Line;
  // TODO: Can we do this in binary mode? (Might require protocol version check)
  while (R.get_raw_line(Line))
    WriteRawLine(Line);

  return *this;
}


void pqxx::tablewriter::WriteRawLine(const string &Line)
{
  Trans().WriteCopyLine(Line);
}


