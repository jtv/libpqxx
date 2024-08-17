/** Implementation of the pqxx::stream_to class.
 *
 * pqxx::stream_to enables optimized batch updates to a database table.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/internal/concat.hxx"
#include "pqxx/internal/gates/connection-stream_to.hxx"
#include "pqxx/stream_from.hxx"
#include "pqxx/stream_to.hxx"

#include "pqxx/internal/header-post.hxx"


namespace
{
using namespace std::literals;

void begin_copy(
  pqxx::transaction_base &tx, std::string_view table, std::string_view columns)
{
  tx.exec(
      std::empty(columns) ?
        pqxx::internal::concat("COPY "sv, table, " FROM STDIN"sv) :
        pqxx::internal::concat(
          "COPY "sv, table, "("sv, columns, ") FROM STDIN"sv))
    .no_rows();
}


/// Return the escape character for escaping the given special character.
char escape_char(char special)
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
  PQXX_UNLIKELY throw pqxx::internal_error{pqxx::internal::concat(
    "Stream escaping unexpectedly stopped at '",
    static_cast<unsigned>(static_cast<unsigned char>(special)), "'.")};
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
  internal::gate::connection_stream_to{m_trans->conn()}.write_copy_line(text);
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
        transaction_focus{tx, s_classname, path},
        m_finder{pqxx::internal::get_char_finder<
          '\b', '\f', '\n', '\r', '\t', '\v', '\\'>(
          pqxx::internal::enc_group(tx.conn().encoding_id()))}
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
    internal::gate::connection_stream_to{m_trans->conn()}.end_copy_write();
  }
}


void pqxx::stream_to::escape_field_to_buffer(std::string_view data)
{
  std::size_t const end{std::size(data)};
  std::size_t here{0};
  while (here < end)
  {
    auto const stop_char{m_finder(data, here)};
    // Append any unremarkable we just skipped over.
    m_buffer.append(std::data(data) + here, stop_char - here);
    if (stop_char < end)
    {
      m_buffer.push_back('\\');
      m_buffer.push_back(escape_char(data[stop_char]));
    }
    here = stop_char + 1;
  }
  // Terminate the field.
  m_buffer.push_back('\t');
}
