/** Implementation of the pqxx::stream_to class.
 *
 * pqxx::stream_to enables optimized batch updates to a database table.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/internal/gates/connection-stream_to.hxx"
#include "pqxx/stream_from.hxx"
#include "pqxx/stream_to.hxx"

#include "pqxx/internal/header-post.hxx"


namespace
{
using namespace std::literals;

void begin_copy(
  pqxx::transaction_base &tx, std::string_view table, std::string_view columns,
  pqxx::sl loc)
{
  pqxx::result res;
  if (std::empty(columns))
    res = tx.exec(std::format("COPY {} FROM STDIN", table));
  else
    res = tx.exec(std::format("COPY {} ({}) FROM STDIN", table, columns));
  res.no_rows(loc);
}


/// Return the escape character for escaping the given special character.
char escape_char(char special, pqxx::sl loc)
{
  switch (special)
  {
  case '\b': return 'b';
  case '\f': return 'f';
  case '\n': return 'n';
  case '\r': return 'r';
  case '\t': return 't';
  case '\v': return 'v';
  case '\\': return '\\';
  default: break;
  }
  throw pqxx::internal_error{
    std::format(
      "Stream escaping unexpectedly stopped at '{}'.",
      static_cast<unsigned>(static_cast<unsigned char>(special))),
    loc};
}
} // namespace


pqxx::stream_to::~stream_to() noexcept
{
  try
  {
    complete(m_created_loc);
  }
  catch (std::exception const &e)
  {
    reg_pending_error(e.what(), m_created_loc);
  }
}


void pqxx::stream_to::write_raw_line(std::string_view text, sl loc)
{
  internal::gate::connection_stream_to{m_trans->conn()}.write_copy_line(
    text, loc);
}


void pqxx::stream_to::write_buffer(sl loc)
{
  if (not std::empty(m_buffer))
  {
    // In append_to_buffer() we write a tab after each field.  We only want a
    // tab _between_ fields.  Remove that last one.
    assert(m_buffer[std::size(m_buffer) - 1] == '\t');
    m_buffer.resize(std::size(m_buffer) - 1);
  }
  write_raw_line(m_buffer, loc);
  m_buffer.clear();
}


pqxx::stream_to &pqxx::stream_to::operator<<(stream_from &tr)
{
  while (tr)
  {
    const auto [line, size] = tr.get_raw_line(m_created_loc);
    if (line.get() == nullptr)
      break;
    write_raw_line(std::string_view{line.get(), size}, m_created_loc);
  }
  return *this;
}


pqxx::stream_to::stream_to(
  transaction_base &tx, std::string_view path, std::string_view columns,
  sl loc) :
        transaction_focus{tx, s_classname, path},
        m_finder{pqxx::internal::get_char_finder<
          '\b', '\f', '\n', '\r', '\t', '\v', '\\'>(
          tx.conn().get_encoding_group(loc), loc)},
        m_created_loc{loc}
{
  begin_copy(tx, path, columns, loc);
  register_me();
}


void pqxx::stream_to::complete(sl loc)
{
  if (!m_finished)
  {
    m_finished = true;
    unregister_me();
    internal::gate::connection_stream_to{m_trans->conn()}.end_copy_write(loc);
  }
}


void pqxx::stream_to::escape_field_to_buffer(std::string_view data, sl loc)
{
  std::size_t const end{std::size(data)};
  std::size_t here{0};
  while (here < end)
  {
    auto const stop_char{m_finder(data, here, loc)};
    // Append any unremarkable characters we just skipped over.
    m_buffer.append(std::data(data) + here, stop_char - here);
    if (stop_char < end)
    {
      m_buffer.push_back('\\');
      m_buffer.push_back(escape_char(data[stop_char], loc));
    }
    here = stop_char + 1;
  }
  // Terminate the field.
  m_buffer.push_back('\t');
}
