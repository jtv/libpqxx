/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_tablereader.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::TableReader class.
 *   Pg::TableReader enables optimized batch reads from a database table
 *
 * Copyright (c) 2001, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pg_tablereader.h"
#include "pg_transaction.h"

using namespace PGSTD;


Pg::TableReader::TableReader(Transaction &T, string RName) :
  TableStream(T, RName),
  m_Done(true)
{
  T.BeginCopyRead(RName);
  m_Done = false;
}


Pg::TableReader::~TableReader()
{
  // If any lines remain to be read, consume them to not confuse PQendcopy()
  string Dummy;
  try
  {
    if (!m_Done) while (Trans().ReadCopyLine(Dummy));
  }
  catch (const exception &e)
  {
    Trans().ProcessNotice(e.what());
  }
}


bool Pg::TableReader::GetRawLine(string &Line)
{
  m_Done = !Trans().ReadCopyLine(Line);
  return !m_Done;
}


