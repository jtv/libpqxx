/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablewriter.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablewriter class.
 *   pqxx::tablewriter enables optimized batch updates to a database table
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


pqxx::tablewriter::tablewriter(transaction_base &T, 
    const string &WName,
    const string &Null) :
  tablestream(T, WName, Null, "tablewriter"),
  m_PendingLine()
{
  T.BeginCopyWrite(WName);
  register_me();
}


pqxx::tablewriter::~tablewriter() throw ()
{
  try
  {
    writer_close();
  }
  catch (const exception &e)
  {
    reg_pending_error(e.what());
  }
}


pqxx::tablewriter &pqxx::tablewriter::operator<<(pqxx::tablereader &R)
{
  string Line;
  // TODO: Can we do this in binary mode? (Might require protocol version check)
  while (R.get_raw_line(Line))
  {
    if (!Line.empty() && (Line[Line.size()-1]=='\n')) Line.erase(Line.size()-1);
    WriteRawLine(Line);
  }

  return *this;
}


void pqxx::tablewriter::WriteRawLine(const string &Line)
{
  /* We use an nonblocking write protocol.  If there is currently no space to
   * write a line to our socket, the tablewriter stores the line being written
   * in a single-line buffer of its own.  This allows the client program to run
   * a little ahead of the socket's transfer capability, hopefully overlapping 
   * some client-side processing with some server-side or networking time.  In
   * principle the tablewriter's buffer could be extended to allow for multiple
   * lines of discrepancy.
   * This does mean that, before writing a new line, we must check for a pending
   * line and send that through first.
   */
  flush_pending();
  if (!m_Trans.WriteCopyLine(Line, true)) m_PendingLine = Line;
}


void pqxx::tablewriter::flush_pending()
{
  if (!m_PendingLine.empty())
  {
    if (!m_Trans.WriteCopyLine(m_PendingLine))
      throw logic_error("libpqxx internal error: "
	  "writing pending line in async mode");
#ifdef PQXX_HAVE_STRING_CLEAR
    m_PendingLine.clear();
#else
    m_PendingLine.resize(0);
#endif
  }
}


void pqxx::tablewriter::complete()
{
  writer_close();
}


void pqxx::tablewriter::writer_close()
{
  flush_pending();
  if (!is_finished())
  {
    base_close();
    try 
    { 
      m_Trans.EndCopyWrite(); 
    } 
    catch (const exception &) 
    {
      try { base_close(); } catch (const exception &) {}
      throw;
    }
  }
}


namespace
{
inline char escapechar(char i)
{
  char r = '\0';
  switch (i)
  {
    case 8:	r='b';	break;	// backspace
    case 11:	r='v';	break;	// vertical tab
    case 12:	r='f';	break;	// form feed
    case '\n':	r='n';	break;	// newline
    case '\t':	r='t';	break;	// tab
    case '\r':	r='r';	break;	// carriage return
    case '\\':	r='\\';	break;	// backslash
  }
  return r;
}

inline bool ishigh(char i)
{
  return (i & 0x80) != 0;
}

inline char tooctdigit(unsigned int i, int n)
{
  return '0' + ((i>>(3*n)) & 0x07);
}
} // namespace


string pqxx::tablewriter::Escape(const string &S)
{
  if (S.empty()) return S;

  string R;
  R.reserve(S.size()+1);

  for (string::const_iterator j = S.begin(); j != S.end(); ++j)
  {
    const char c = *j;
    const char e = escapechar(c);
    if (e)
    {
      R += '\\';
      R += e;
    }
    else if (ishigh(c))
    {
      R += '\\';
      unsigned char u=c;
      for (int n=2; n>=0; --n) R += tooctdigit(u,n);
    }
    else
    {
      R += c;
    }
  }
  return R;
}


