/** Implementation of the pqxx::stream_to class.
 *
 * pqxx::stream_to enables optimized batch updates to a database table.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/stream_from.hxx"
#include "pqxx/stream_to.hxx"

#include "pqxx/internal/concat.hxx"
#include "pqxx/internal/gates/connection-stream_to.hxx"

namespace
{
using namespace std::literals;

void begin_copy(
  pqxx::transaction_base &tx, std::string_view table, std::string_view columns)
{
  tx.exec0(
    std::empty(columns) ?
      pqxx::internal::concat("COPY "sv, table, " FROM STDIN"sv) :
      pqxx::internal::concat(
        "COPY "sv, table, "("sv, columns, ") FROM STDIN"sv));
}
} // namespace


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
  if (not std::empty(m_buffer))
  {
    // In append_to_buffer() we write a tab after each field.  We only want a
    // tab _between_ fields.  Remove that last one.
    assert(m_buffer[std::size(m_buffer) - 1] == '\t');
    m_buffer.resize(std::size(m_buffer) - 1);
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


pqxx::stream_to::stream_to(
  transaction_base &tx, std::string_view path, std::string_view columns) :
        transaction_focus{tx, s_classname, path}
{
  begin_copy(tx, path, columns);
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
