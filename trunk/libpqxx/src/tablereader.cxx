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
#include "pqxx/compiler.h"

#include "pqxx/tablereader"
#include "pqxx/transaction"

using namespace PGSTD;


pqxx::tablereader::tablereader(transaction_base &T, 
    const string &RName,
    const string &Null) :
  tablestream(T, RName, Null),
  m_Done(true)
{
  T.BeginCopyRead(RName);
  m_Done = false;
}


pqxx::tablereader::~tablereader()
{
  try
  {
    reader_close();
  }
  catch (const exception &e)
  {
    Trans().RegisterPendingError(e.what());
  }
}


bool pqxx::tablereader::get_raw_line(string &Line)
{
  if (!m_Done) try
  {
    m_Done = !Trans().ReadCopyLine(Line);
  }
  catch (const exception &)
  {
    m_Done = true;
    throw;
  }
  return !m_Done;
}


void pqxx::tablereader::complete()
{
  reader_close();
}


void pqxx::tablereader::reader_close()
{
  if (!is_finished())
  {
    // If any lines remain to be read, consume them to not confuse PQendcopy()
    if (!m_Done)
    {
      try
      {
        string Dummy;
        while (get_raw_line(Dummy));
      }
      catch (const broken_connection &)
      {
	try { base_close(); } catch (const exception &) {}
	throw;
      }
      catch (const exception &e)
      {
        RegisterPendingError(e.what());
      }
    }
    base_close();
  }
}


char pqxx::tablereader::unescapechar(char i) throw ()
{
  char r = i;
  switch (i)
  {
    case 'b':	r=8;	break;	// backspace
    case 'v':	r=11;	break;	// vertical tab
    case 'f':	r=12;	break;	// form feed
    case 'n':	r='\n';	break;	// newline
    case 't':	r='\t';	break;	// tab
    case 'r':	r='\r';	break;	// carriage return;
  }
  return r;
}



