/** Implementation of the pqxx::tablereader2 class.
 *
 * pqxx::tablereader2 enables optimized batch reads from a database table.
 *
 * Copyright (c) 2001-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/tablereader2"

#include "pqxx/internal/encodings.hxx"
#include "pqxx/internal/gates/transaction-tablereader2.hxx"


namespace
{
  // bool is_octalchar(char o) noexcept
  // {
  //   return (o>='0') && (o<='7');
  // }

  /// Find first tab character at or after start position in string
  /** If not found, returns line.size() rather than string::npos.
   */
  std::string::size_type find_tab(
    pqxx::internal::encoding_group enc,
    const std::string &line,
    std::string::size_type start
  )
  {
    using namespace pqxx::internal;
    auto here = find_with_encoding(enc, line, '\t', start);
    return (here == std::string::npos) ? line.size() : here;
  }
} // namespace


pqxx::tablereader2::tablereader2(
  transaction_base &tb,
  const std::string &table_name
) :
  namedclass("tablereader2", table_name),
  tablestream2(tb)
{
  setup(tb, table_name);
}


pqxx::tablereader2::~tablereader2() noexcept
{
  try
  {
    complete();
  }
  catch (const std::exception &e)
  {
    reg_pending_error(e.what());
  }
}


void pqxx::tablereader2::complete()
{
  close();
}


bool pqxx::tablereader2::get_raw_line(std::string &line)
{
  if (*this)
    try
    {
      m_finished = !internal::gate::transaction_tablereader2{
        m_trans
      }.read_copy_line(
        line
      );
    }
    catch (const std::exception &)
    {
      m_finished = true;
      throw;
    }
  return *this;
}


void pqxx::tablereader2::setup(
  transaction_base &tb,
  const std::string &table_name
)
{
  setup(tb, table_name, "");
}


void pqxx::tablereader2::setup(
  transaction_base &tb,
  const std::string &table_name,
  const std::string &columns
)
{
  // Get the encoding before starting the COPY, otherwise reading the the
  // variable will interrupt it
  m_copy_encoding = internal::gate::transaction_tablereader2{
    m_trans
  }.current_encoding();
  internal::gate::transaction_tablereader2{tb}.BeginCopyRead(
    table_name,
    columns
  );
  register_me();
}


void pqxx::tablereader2::close()
{
  pqxx::tablestream2::close();
  try
  {
    // Flush any remaining lines
    std::string s;
    while (get_raw_line(s));
  }
  catch (const broken_connection &)
  {
    try
    {
      pqxx::tablestream2::close();
    }
    catch (const std::exception &) {}
    throw;
  }
  catch (const std::exception &e)
  {
    reg_pending_error(e.what());
  }
}


bool pqxx::tablereader2::extract_field(
  const std::string &line,
  std::string::size_type &i,
  std::string &s
) const
{
  using namespace pqxx::internal;

  s.clear();
  bool is_null{false};
  auto stop = find_tab(m_copy_encoding, line, i);
  while (i < stop)
  {
    auto here = next_seq(m_copy_encoding, line.c_str(), line.size(), i);
    auto seq_len = here.end_byte - here.begin_byte;
    if (seq_len == 1)
    {
      switch (line[here.begin_byte])
      {
      case '\n':
        // End-of-row; shouldn't happen, but we may get old-style
        // newline-terminated lines
        i = stop;
        break;

      case '\\':
        {
        // Escape sequence
          if (here.end_byte >= line.size())
            throw failure{"Row ends in backslash"};
          char n = line[here.end_byte++];
          
          // if (is_octalchar(n))
          // {
          //   if (here.end_byte+2 >= line.size())
          //     throw failure{"Row ends in middle of octal value"};
          //   char n1 = line[here.end_byte++];
          //   char n2 = line[here.end_byte++];
          //   if (!is_octalchar(n1) || !is_octalchar(n2))
          //     throw failure{
          //       "Invalid octal in encoded table stream"
          //     };
          //   s += (
          //     (digit_to_number(n)<<6) |
          //     (digit_to_number(n1)<<3) |
          //     digit_to_number(n2)
          //   );
          //   break;
          // }
          // else
            switch (n)
            {
            case 'N':
              // Null value
              if (!s.empty())
                throw failure{
                  "Null sequence found in nonempty field"
                };
              is_null = true;
              break;

            case 'b': // Backspace
              s += '\b'; break;
            case 'f': // Vertical tab
              s += '\f'; break;
            case 'n': // Form feed
              s += '\n'; break;
            case 'r': // Newline
              s += '\r'; break;
            case 't': // Tab
              s += '\t'; break;
            case 'v': // Carriage return
              s += '\v'; break;

            default:
              // Self-escaped character
              s += n;
              break;
            }
        }
        break;

      default:
        s += line[here.begin_byte];
        break;
      }
    }
    else
    {
      // Multi-byte sequence; never treated specially, so just append
      s.insert(s.size(), line.c_str() + here.begin_byte, seq_len);
    }

    i = here.end_byte;
  }

  // Skip field separator
  i += 1;

  return !is_null;
}
