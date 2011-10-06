/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablereader.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablereader class.
 *   pqxx::tablereader enables optimized batch reads from a database table
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#ifdef PQXX_QUIET_DESTRUCTORS
#include "pqxx/errorhandler"
#endif

#include "pqxx/tablereader"
#include "pqxx/transaction"

#include "pqxx/internal/gates/transaction-tablereader.hxx"

using namespace PGSTD;
using namespace pqxx::internal;


pqxx::tablereader::tablereader(transaction_base &T,
    const PGSTD::string &Name,
    const PGSTD::string &Null) :
  namedclass("tablereader", Name),
  tablestream(T, Null),
  m_Done(true)
{
  setup(T, Name);
}

void pqxx::tablereader::setup(transaction_base &T,
    const PGSTD::string &Name,
    const PGSTD::string &Columns)
{
  gate::transaction_tablereader(T).BeginCopyRead(Name, Columns);
  register_me();
  m_Done = false;
}

pqxx::tablereader::~tablereader() throw ()
{
#ifdef PQXX_QUIET_DESTRUCTORS
  quiet_errorhandler quiet(m_Trans.conn());
#endif
  try
  {
    reader_close();
  }
  catch (const exception &e)
  {
    reg_pending_error(e.what());
  }
}


bool pqxx::tablereader::get_raw_line(PGSTD::string &Line)
{
  if (!m_Done) try
  {
    m_Done = !gate::transaction_tablereader(m_Trans).ReadCopyLine(Line);
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
        while (get_raw_line(Dummy)) ;
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

/// Find first tab character at or after start position in string
/** If not found, returns Line.size() rather than string::npos.
 */
string::size_type findtab(const PGSTD::string &Line,
	PGSTD::string::size_type start)
{
  // TODO: Fix for multibyte encodings?
  const string::size_type here = Line.find('\t', start);
  return (here == string::npos) ? Line.size() : here;
}
} // namespace


string pqxx::tablereader::extract_field(const PGSTD::string &Line,
    PGSTD::string::size_type &i) const
{
  // TODO: Pick better exception types
  string R;
  bool isnull=false;
  string::size_type stop = findtab(Line, i);
  for (; i < stop; ++i)
  {
    const char c = Line[i];
    switch (c)
    {
    case '\n':			// End of row
      // Shouldn't happen, but we may get old-style, newline-terminated lines
      i = stop;
      break;

    case '\\':			// Escape sequence
      {
        const char n = Line[++i];
        if (i >= Line.size())
          throw failure("Row ends in backslash");

	switch (n)
	{
	case 'N':	// Null value
	  if (!R.empty())
	    throw failure("Null sequence found in nonempty field");
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
	      throw failure("Row ends in middle of octal value");
	    const char n1 = Line[++i];
	    const char n2 = Line[++i];
	    if (!is_octalchar(n1) || !is_octalchar(n2))
	      throw failure("Invalid octal in encoded table stream");
	    R += char((digit_to_number(n)<<6) |
		(digit_to_number(n1)<<3) |
		digit_to_number(n2));
          }
	  break;

	case 'b':
	  // TODO: Escape code?
	  R += char(8);
	  break;	// Backspace
	case 'v':
	  // TODO: Escape code?
	  R += char(11);
	  break;	// Vertical tab
	case 'f':
	  // TODO: Escape code?
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
	  // This may be a self-escaped tab that we thought was a terminator...
	  if (i == stop)
	  {
	    if ((i+1) >= Line.size())
	      throw internal_error("COPY line ends in backslash");
	    stop = findtab(Line, i+1);
	  }
	  break;
	}
      }
      break;

    default:
      R += c;
      break;
    }
  }
  ++i;

  if (isnull && (R.size() != NullStr().size()))
    throw failure("Field contains data behind null sequence");

  return R;
}

