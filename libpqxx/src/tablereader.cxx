/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablereader.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablereader class.
 *   pqxx::tablereader enables optimized batch reads from a database table
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
#include "pqxx/transaction"

using namespace PGSTD;


pqxx::tablereader::tablereader(transaction_base &T, 
    const string &RName,
    const string &Null) :
  tablestream(T, RName, Null, "tablereader"),
  m_Done(true)
{
  T.BeginCopyRead(RName);
  register_me();
  m_Done = false;
}


pqxx::tablereader::~tablereader() throw ()
{
  try
  {
    reader_close();
  }
  catch (const exception &e)
  {
    reg_pending_error(e.what());
  }
}


bool pqxx::tablereader::get_raw_line(string &Line)
{
  if (!m_Done) try
  {
    m_Done = !m_Trans.ReadCopyLine(Line);
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
    base_close();

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
        reg_pending_error(e.what());
      }
    }
  }
}


namespace
{
inline bool is_octalchar(char o) throw ()
{
  return (o>='0') && (o<='7');
}
} // namespace


string pqxx::tablereader::extract_field(const string &Line,
    string::size_type &i) const
{
  // TODO: Pick better exception types
  string R;
  bool isnull=false, terminator=false;
  for (; !terminator && (i < Line.size()); ++i)
  {
    const char c = Line[i];
    switch (c)
    {
    case '\t':			// End of field
    case '\n':			// End of row
      terminator = true;
      break;

    case '\\':			// Escape sequence
      {
        const char n = Line[++i];
        if (i >= Line.size())
          throw runtime_error("Row ends in backslash");

	switch (n)
	{
	case 'N':	// Null value
	  if (!R.empty())
	    throw runtime_error("Null sequence found in nonempty field");
	  R = NullStr();
	  isnull = true;
	  break;
        
	case '0':	// Octal sequence (3 digits)
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
          {
	    if ((i+2) >= Line.size())
	      throw runtime_error("Row ends in middle of octal value");
	    const char n1 = Line[++i];
	    const char n2 = Line[++i];
	    if (!is_octalchar(n1) || !is_octalchar(n2))
	      throw runtime_error("Invalid octal in encoded table stream");
	    R += char(((n-'0')<<6) | ((n1-'0')<<3) | (n2-'0'));
          }
	  break;

    	case 'b':
	  R += char(8);
	  break;	// Backspace
    	case 'v':
	  R += char(11);
	  break;	// Vertical tab
    	case 'f':
	  R += char(12);
	  break;	// Form feed
    	case 'n':
	  R += '\n';
	  break;	// Newline
    	case 't':
	  R += '\t';
	  break;	// Tab
    	case 'r':
	  R += '\r';
	  break;	// Carriage return;

	default:	// Self-escaped character
	  R += n;
	  break;
	}
      }
      break;

    default:
      R += c;
      break;
    }
  }

  if (isnull && (R.size() != NullStr().size()))
    throw runtime_error("Field contains data behind null sequence");

  return R;
}

