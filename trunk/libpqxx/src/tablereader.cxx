/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablereader.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablereader class.
 *   pqxx::tablereader enables optimized batch reads from a database table
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
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::tablereader::tablereader(transaction_base &T, const string &RName) :
  tablestream(T, RName),
  m_Done(true)
{
  T.BeginCopyRead(RName);
  m_Done = false;
}


pqxx::tablereader::~tablereader()
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


bool pqxx::tablereader::GetRawLine(string &Line)
{
  m_Done = !Trans().ReadCopyLine(Line);
  return !m_Done;
}


