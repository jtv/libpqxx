/** Implementation of the pqxx::stream_from class.
 *
 * pqxx::stream_from enables optimized batch reads from a database table.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/stream_from"

#include "pqxx/internal/encodings.hxx"
#include "pqxx/internal/gates/connection-stream_from.hxx"


namespace
{
/// Find first tab character at or after start position in string.
/** If not found, returns line.size() rather than string::npos.
 */
std::string::size_type find_tab(
  pqxx::internal::encoding_group enc, const std::string &line,
  std::string::size_type start)
{
  auto here = pqxx::internal::find_with_encoding(enc, line, '\t', start);
  return (here == std::string::npos) ? line.size() : here;
}


void begin_copy(
  pqxx::transaction_base &trans, std::string_view table,
  const std::string &columns)
{
  constexpr std::string_view copy{"COPY "}, to_stdout{" TO STDOUT"};
  std::string query;
  query.reserve(
    copy.size() + table.size() + columns.size() + 2 + to_stdout.size());
  query += copy;
  query += table;

  if (not columns.empty())
  {
    query.push_back('(');
    query += columns;
    query.push_back(')');
  }

  query += to_stdout;

  trans.exec0(query);
}
} // namespace


pqxx::stream_from::stream_from(
  transaction_base &tb, std::string_view table_name) :
        namedclass{"stream_from", table_name},
        transactionfocus{tb}
{
  set_up(tb, table_name);
}


pqxx::stream_from::~stream_from() noexcept
{
  try
  {
    close();
  }
  catch (const std::exception &e)
  {
    reg_pending_error(e.what());
  }
}


bool pqxx::stream_from::get_raw_line(std::string &line)
{
  internal::gate::connection_stream_from gate{m_trans.conn()};
  if (*this)
  {
    try
    {
      if (not gate.read_copy_line(line))
        close();
    }
    catch (const std::exception &)
    {
      close();
      throw;
    }
  }
  return *this;
}


void pqxx::stream_from::set_up(
  transaction_base &tb, std::string_view table_name)
{
  set_up(tb, table_name, "");
}


void pqxx::stream_from::set_up(
  transaction_base &tb, std::string_view table_name,
  const std::string &columns)
{
  // Get the encoding before starting the COPY, otherwise reading the the
  // variable will interrupt it
  m_copy_encoding = internal::enc_group(m_trans.conn().encoding_id());
  begin_copy(tb, table_name, columns);
  register_me();
}


void pqxx::stream_from::close()
{
  if (!m_finished)
  {
    m_finished = true;
    unregister_me();
  }
}


void pqxx::stream_from::complete()
{
  if (m_finished)
    return;
  try
  {
    // Flush any remaining lines - libpq will automatically close the stream
    // when it hits the end.
    std::string s;
    while (get_raw_line(s))
      ;
  }
  catch (const broken_connection &)
  {
    close();
    throw;
  }
  catch (const std::exception &e)
  {
    reg_pending_error(e.what());
  }
  close();
}


bool pqxx::stream_from::extract_field(
  const std::string &line, std::string::size_type &i, std::string &s) const
{
  const auto next_seq = get_glyph_scanner(m_copy_encoding);
  s.clear();
  bool is_null{false};
  auto stop = find_tab(m_copy_encoding, line, i);
  while (i < stop)
  {
    auto glyph_end = next_seq(line.c_str(), line.size(), i);
    if (auto seq_len = glyph_end - i; seq_len == 1)
    {
      switch (line[i])
      {
      case '\n':
        // End-of-row; shouldn't happen, but we may get old-style
        // newline-terminated lines.
        i = stop;
        break;

      case '\\':
      {
        // Escape sequence.
        if (glyph_end >= line.size())
          throw failure{"Row ends in backslash"};
        char n = line[glyph_end++];
        switch (n)
        {
        case 'N':
          // Null value
          if (not s.empty())
            throw failure{"Null sequence found in nonempty field"};
          is_null = true;
          break;

        case 'b': // Backspace
          s += '\b';
          break;
        case 'f': // Vertical tab
          s += '\f';
          break;
        case 'n': // Form feed
          s += '\n';
          break;
        case 'r': // Newline
          s += '\r';
          break;
        case 't': // Tab
          s += '\t';
          break;
        case 'v': // Carriage return
          s += '\v';
          break;

        default:
          // Self-escaped character
          s += n;
          break;
        }
      }
      break;

      default: s += line[i]; break;
      }
    }
    else
    {
      // Multi-byte sequence; never treated specially, so just append
      s.insert(s.size(), line.c_str() + i, seq_len);
    }

    i = glyph_end;
  }

  // Skip field separator
  i += 1;

  return not is_null;
}

template<>
void pqxx::stream_from::extract_value<std::nullptr_t>(
  const std::string &line, std::nullptr_t &, std::string::size_type &here,
  std::string &workspace) const
{
  if (extract_field(line, here, workspace))
    throw pqxx::conversion_error{"Attempt to convert non-null '" + workspace +
                                 "' to null"};
}
