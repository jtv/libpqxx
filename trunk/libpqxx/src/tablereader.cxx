/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablereader.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::TableReader class.
 *   pqxx::TableReader enables optimized batch reads from a database table
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/tablereader.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::TableReader::TableReader(TransactionItf &T, const string &RName) :
  TableStream(T, RName),
  m_Done(true)
{
  T.BeginCopyRead(RName);
  m_Done = false;
}


pqxx::TableReader::~TableReader()
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


bool pqxx::TableReader::GetRawLine(string &Line)
{
  m_Done = !Trans().ReadCopyLine(Line);
  return !m_Done;
}


