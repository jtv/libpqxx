/** Implementation of the pqxx::tablereader class.
 *
 * pqxx::tablereader enables optimized batch reads from a database table.
 *
 * Copyright (c) 2001-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/tablereader"
#include "pqxx/transaction"

#include "pqxx/internal/encodings.hxx"
#include "pqxx/internal/gates/transaction-tablereader.hxx"

using namespace pqxx::internal;


pqxx::tablereader::tablereader(
	transaction_base &T,
	const std::string &Name,
	const std::string &Null) :
  namedclass("tablereader", Name),
  tablestream(T, Null),
  m_done(true)
{
  setup(T, Name);
}

void pqxx::tablereader::setup(
	transaction_base &T,
	const std::string &Name,
	const std::string &Columns)
{
  // Get the encoding before starting the COPY, otherwise reading the the
  // variable will interrupt it
  m_copy_encoding = gate::transaction_tablereader(m_trans).current_encoding();
  gate::transaction_tablereader(T).BeginCopyRead(Name, Columns);
  register_me();
  m_done = false;
}

pqxx::tablereader::~tablereader() noexcept
{
  try
  {
    reader_close();
  }
  catch (const std::exception &e)
  {
    reg_pending_error(e.what());
  }
}


bool pqxx::tablereader::get_raw_line(std::string &Line)
{
  if (!m_done) try
  {
    m_done = !gate::transaction_tablereader(m_trans).read_copy_line(Line);
  }
  catch (const std::exception &)
  {
    m_done = true;
    throw;
  }
  return !m_done;
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
    if (!m_done)
    {
      try
      {
        std::string Dummy;
        while (get_raw_line(Dummy)) ;
      }
      catch (const broken_connection &)
      {
	try { base_close(); } catch (const std::exception &) {}
	throw;
      }
      catch (const std::exception &e)
      {
        reg_pending_error(e.what());
      }
    }
  }
}


namespace
{
inline bool is_octalchar(char o) noexcept
{
  return (o>='0') && (o<='7');
}

/// Find first tab character at or after start position in string
/** If not found, returns Line.size() rather than string::npos.
 */
std::string::size_type findtab(
  pqxx::internal::encoding_group enc,
  const std::string &Line,
  std::string::size_type start)
{
  using namespace pqxx::internal;
  auto here = find_with_encoding(enc, Line, '\t', start);
  return (here == std::string::npos) ? Line.size() : here;
}
} // namespace


std::string pqxx::tablereader::extract_field(
	const std::string &Line,
	std::string::size_type &i) const
{
  std::string s;
  if (extract_field(Line, i, s))
    return s;
  else
    return NullStr();
}

bool pqxx::tablereader::extract_field(
	const std::string &Line,
	std::string::size_type &i,
	std::string &s) const
{
  using namespace pqxx::internal;
  // TODO: Pick better exception types
  s.clear();
  bool isnull=false;
  auto stop = findtab(m_copy_encoding, Line, i);
  while (i < stop)
  {
    auto here = next_seq(m_copy_encoding, Line.c_str(), Line.size(), i);
    auto seq_len = here.end_byte - here.begin_byte;
    if (seq_len == 1)
    {
      switch (Line[here.begin_byte])
      {
      case '\n':                // End of row
        // Shouldn't happen, but we may get old-style, newline-terminated lines
        i = stop;
        break;

      case '\\':                // Escape sequence
        {
          if (i >= Line.size())
            throw failure("Row ends in backslash");
          const char n = Line[++i];

          switch (n)
          {
          case 'N':     // Null value
            if (!s.empty())
              throw failure("Null sequence found in nonempty field");
            s = NullStr();
            isnull = true;
            break;

          case '0':     // Octal sequence (3 digits)
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
              s += char(
                  (digit_to_number(n)<<6) |
                  (digit_to_number(n1)<<3) |
                  digit_to_number(n2));
            }
            break;

          case 'b':     // Backspace
            s += '\b';
            break;
          case 'v':     // Vertical tab
            s += '\v';
            break;
          case 'f':     // Form feed
            s += '\f';
            break;
          case 'n':     // Newline
            s += '\n';
            break;
          case 't':     // Tab
            s += '\t';
            break;
          case 'r':     // Carriage return;
            s += '\r';
            break;

          default:      // Self-escaped character
            s += n;
            // This may be a self-escaped tab that we thought was a terminator
            if (i == stop)
            {
              if ((i+1) >= Line.size())
                throw internal_error("COPY line ends in backslash");
              stop = findtab(m_copy_encoding, Line, i+1);
            }
            break;
          }
        }
        break;

      default:
        s += Line[here.begin_byte];
        break;
      }
      ++i;
    }
    else // Multi-byte sequence; never treated specially, so just append
    {
      s.insert(s.size(), Line.c_str() + here.begin_byte, seq_len);
      i += seq_len;
    }
  }

  // Skip field separator
  ++i;

  if (isnull && (s.size() != NullStr().size()))
    throw failure("Field contains data behind null sequence");

  return !isnull;
}
