/** Implementation of the pqxx::stream_to class.
 *
 * pqxx::stream_to enables optimized batch updates to a database table.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/stream_from.hxx"
#include "pqxx/stream_to.hxx"

#include "pqxx/internal/gates/connection-stream_to.hxx"


namespace
{
void begin_copy(
  pqxx::transaction_base &trans, std::string_view table,
  std::string const &columns)
{
  constexpr std::string_view copy{"COPY "}, from_stdin{" FROM STDIN"};
  std::string query;
  query.reserve(
    copy.size() + table.size() + 2 + columns.size() + from_stdin.size());

  query += copy;
  query += table;
  if (not columns.empty())
  {
    query.push_back('(');
    query += columns;
    query.push_back(')');
  }
  query += from_stdin;

  trans.exec0(query);
}
} // namespace


pqxx::stream_to::stream_to(transaction_base &tb, std::string_view table_name) :
        namedclass{"stream_to", table_name}, internal::transactionfocus{tb}
{
  set_up(tb, table_name);
}


pqxx::stream_to::~stream_to() noexcept
{
  try
  {
    complete();
  }
  catch (std::exception const &e)
  {
    reg_pending_error(e.what());
  }
}


void pqxx::stream_to::write_raw_line(std::string_view text)
{
  internal::gate::connection_stream_to{m_trans.conn()}.write_copy_line(text);
}


void pqxx::stream_to::write_buffer()
{
  if (not m_buffer.empty())
  {
    // In append_to_buffer() we write a tab after each field.  We only want a
    // tab _between_ fields.  Remove that last one.
    assert(m_buffer[m_buffer.size() - 1] == '\t');
    m_buffer.resize(m_buffer.size() - 1);
  }
  write_raw_line(m_buffer);
  m_buffer.clear();
}


pqxx::stream_to &pqxx::stream_to::operator<<(stream_from &tr)
{
  while (tr)
  {
    const auto [line, size] = tr.get_raw_line();
    if (line.get() == nullptr)
      break;
    write_raw_line(std::string_view{line.get(), size});
  }
  return *this;
}


void pqxx::stream_to::set_up(transaction_base &tb, std::string_view table_name)
{
  set_up(tb, table_name, "");
}


void pqxx::stream_to::set_up(
  transaction_base &tb, std::string_view table_name,
  std::string const &columns)
{
  begin_copy(tb, table_name, columns);
  register_me();
}


void pqxx::stream_to::complete()
{
  if (!m_finished)
  {
    m_finished = true;
    unregister_me();
    internal::gate::connection_stream_to{m_trans.conn()}.end_copy_write();
  }
}


void pqxx::stream_to::escape_field_to_buffer(std::string_view buf)
{
  for (auto c : buf)
  {
    switch (c)
    {
    case '\b': m_buffer += "\\b"; break;  // Backspace
    case '\f': m_buffer += "\\f"; break;  // Vertical tab
    case '\n': m_buffer += "\\n"; break;  // Form feed
    case '\r': m_buffer += "\\r"; break;  // Newline
    case '\t': m_buffer += "\\t"; break;  // Tab
    case '\v': m_buffer += "\\v"; break;  // Carriage return
    case '\\': m_buffer += "\\\\"; break; // Backslash
    default:
      if (c < ' ' or c > '~')
      {
        // Non-ASCII.  Escape as octal number.
        m_buffer += "\\";
        auto u{static_cast<unsigned char>(c)};
        for (auto i = 2; i >= 0; --i)
          m_buffer += pqxx::internal::number_to_digit((u >> (3 * i)) & 0x07);
      }
      else
      {
        m_buffer += c;
      }
      break;
    }
  }
  m_buffer += '\t';
}
