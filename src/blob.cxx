#include "pqxx-source.hxx"

#include <cerrno>
#include <stdexcept>
#include <utility>

#include "pqxx/internal/header-pre.hxx"

#include <libpq-fe.h>

#include "pqxx/blob.hxx"
#include "pqxx/except.hxx"
#include "pqxx/internal/gates/connection-largeobject.hxx"

#include "pqxx/internal/header-post.hxx"


namespace
{
constexpr int INV_WRITE{0x00020000}, INV_READ{0x00040000};
} // namespace


pqxx::internal::pq::PGconn *pqxx::blob::raw_conn(pqxx::connection *cx) noexcept
{
  pqxx::internal::gate::connection_largeobject const gate{*cx};
  return gate.raw_connection();
}


pqxx::internal::pq::PGconn *
pqxx::blob::raw_conn(pqxx::dbtransaction const &tx) noexcept
{
  return raw_conn(&tx.conn());
}


std::string pqxx::blob::errmsg(connection const *cx)
{
  pqxx::internal::gate::const_connection_largeobject const gate{*cx};
  return gate.error_message();
}


pqxx::blob
pqxx::blob::open_internal(dbtransaction &tx, oid id, int mode, sl loc)
{
  auto &cx{tx.conn()};
  int const fd{lo_open(raw_conn(&cx), id, mode)};
  if (fd == -1)
    throw pqxx::failure{
      std::format(
        "Could not open binary large object {}: {}", id, errmsg(&cx)),
      loc};
  return {cx, fd};
}


pqxx::oid pqxx::blob::create(dbtransaction &tx, oid id, sl loc)
{
  oid const actual_id{lo_create(raw_conn(tx), id)};
  if (actual_id == 0)
    throw failure{
      std::format(
        "Could not create binary large object: {}", errmsg(&tx.conn())),
      loc};
  return actual_id;
}


void pqxx::blob::remove(dbtransaction &tx, oid id, sl loc)
{
  if (id == 0)
    throw usage_error{
      "Trying to delete binary large object without an ID.", loc};
  if (lo_unlink(raw_conn(tx), id) == -1)
    throw failure{
      std::format(
        "Could not delete large object {}: {}", id, errmsg(&tx.conn())),
      loc};
}


pqxx::blob pqxx::blob::open_r(dbtransaction &tx, oid id, sl loc)
{
  return open_internal(tx, id, INV_READ, loc);
}


pqxx::blob pqxx::blob::open_w(dbtransaction &tx, oid id, sl loc)
{
  return open_internal(tx, id, INV_WRITE, loc);
}


pqxx::blob pqxx::blob::open_rw(dbtransaction &tx, oid id, sl loc)
{
  return open_internal(tx, id, INV_READ | INV_WRITE, loc);
}


pqxx::blob::blob(blob &&other) :
        m_conn{std::exchange(other.m_conn, nullptr)},
        m_fd{std::exchange(other.m_fd, -1)}
{}


pqxx::blob &pqxx::blob::operator=(blob &&other)
{
  if (m_fd != -1)
    lo_close(raw_conn(m_conn), m_fd);
  m_conn = std::exchange(other.m_conn, nullptr);
  m_fd = std::exchange(other.m_fd, -1);
  return *this;
}


pqxx::blob::~blob()
{
  try
  {
    close();
  }
  catch (std::exception const &e)
  {
    if (m_conn != nullptr)
    {
      m_conn->process_notice("Failure while closing binary large object:\n");
      // TODO: Make at least an attempt to append a newline.
      m_conn->process_notice(e.what());
    }
  }
}


void pqxx::blob::close()
{
  if (m_fd != -1)
  {
    lo_close(raw_conn(m_conn), m_fd);
    m_fd = -1;
    m_conn = nullptr;
  }
}


std::size_t pqxx::blob::raw_read(std::byte buf[], std::size_t size, sl loc)
{
  if (m_conn == nullptr)
    throw usage_error{
      "Attempt to read from a closed binary large object.", loc};
  if (size > chunk_limit)
    throw range_error{
      "Reads from a binary large object must be less than 2 GB at once.", loc};
  auto data{reinterpret_cast<char *>(buf)};
  int const received{lo_read(raw_conn(m_conn), m_fd, data, size)};
  if (received < 0)
    throw failure{
      std::format("Could not read from binary large object: {}", errmsg()),
      loc};
  return static_cast<std::size_t>(received);
}


std::size_t pqxx::blob::read(bytes &buf, std::size_t size, sl loc)
{
  buf.resize(size);
  auto const received{raw_read(std::data(buf), size, loc)};
  buf.resize(received);
  return static_cast<std::size_t>(received);
}


void pqxx::blob::raw_write(bytes_view data, sl loc)
{
  if (m_conn == nullptr)
    throw usage_error{
      "Attempt to write to a closed binary large object.", loc};
  auto const sz{std::size(data)};
  if (sz > chunk_limit)
    throw range_error{"Write to binary large object exceeds 2GB limit.", loc};
  auto ptr{reinterpret_cast<char const *>(std::data(data))};
  int const written{lo_write(raw_conn(m_conn), m_fd, ptr, sz)};
  if (written < 0)
    throw failure{
      std::format("Write to binary large object failed: {}", errmsg()), loc};
}


void pqxx::blob::resize(std::int64_t size, sl loc)
{
  if (m_conn == nullptr)
    throw usage_error{"Attempt to resize a closed binary large object.", loc};
  if (lo_truncate64(raw_conn(m_conn), m_fd, size) < 0)
    throw failure{
      std::format("Binary large object truncation failed: {}", errmsg()), loc};
}


std::int64_t pqxx::blob::tell(sl loc) const
{
  if (m_conn == nullptr)
    throw usage_error{"Attempt to tell() a closed binary large object.", loc};
  std::int64_t const offset{lo_tell64(raw_conn(m_conn), m_fd)};
  if (offset < 0)
    throw failure{
      std::format("Error reading binary large object position: {}", errmsg()),
      loc};
  return offset;
}


std::int64_t pqxx::blob::seek(std::int64_t offset, int whence, sl loc)
{
  if (m_conn == nullptr)
    throw usage_error{"Attempt to seek() a closed binary large object.", loc};
  std::int64_t const seek_result{
    lo_lseek64(raw_conn(m_conn), m_fd, offset, whence)};
  if (seek_result < 0)
    throw failure{
      std::format("Error during seek on binary large object: {}", errmsg()),
      loc};
  return seek_result;
}


std::int64_t pqxx::blob::seek_abs(std::int64_t offset, sl loc)
{
  return this->seek(offset, SEEK_SET, loc);
}


std::int64_t pqxx::blob::seek_rel(std::int64_t offset, sl loc)
{
  return this->seek(offset, SEEK_CUR, loc);
}


std::int64_t pqxx::blob::seek_end(std::int64_t offset, sl loc)
{
  return this->seek(offset, SEEK_END, loc);
}


pqxx::oid
pqxx::blob::from_buf(dbtransaction &tx, bytes_view data, oid id, sl loc)
{
  oid const actual_id{create(tx, id, loc)};
  try
  {
    open_w(tx, actual_id, loc).write(data, loc);
  }
  catch (std::exception const &)
  {
    try
    {
      remove(tx, id, loc);
    }
    catch (std::exception const &e)
    {
      try
      {
        tx.conn().process_notice(std::format(
          "Could not clean up partially created large object {}: {}\n", id,
          e.what()));
      }
      catch (std::exception const &)
      {}
    }
    throw;
  }
  return actual_id;
}


void pqxx::blob::append_from_buf(
  dbtransaction &tx, bytes_view data, oid id, sl loc)
{
  if (std::size(data) > chunk_limit)
    throw range_error{
      "Writes to a binary large object must be less than 2 GB at once.", loc};
  blob b{open_w(tx, id, loc)};
  b.seek_end(0, loc);
  b.write(data, loc);
}


void pqxx::blob::to_buf(
  dbtransaction &tx, oid id, bytes &buf, std::size_t max_size, sl loc)
{
  open_r(tx, id, loc).read(buf, max_size, loc);
}


std::size_t pqxx::blob::append_to_buf(
  dbtransaction &tx, oid id, std::int64_t offset, bytes &buf,
  std::size_t append_max, sl loc)
{
  if (append_max > chunk_limit)
    throw range_error{
      "Reads from a binary large object must be less than 2 GB at once.", loc};
  auto b{open_r(tx, id, loc)};
  b.seek_abs(offset, loc);
  auto const org_size{std::size(buf)};
  buf.resize(org_size + append_max);
  try
  {
    auto here{reinterpret_cast<char *>(std::data(buf) + org_size)};
    auto chunk{static_cast<std::size_t>(
      lo_read(raw_conn(b.m_conn), b.m_fd, here, append_max))};
    buf.resize(org_size + chunk);
    return chunk;
  }
  catch (std::exception const &)
  {
    buf.resize(org_size);
    throw;
  }
}


pqxx::oid pqxx::blob::from_file(dbtransaction &tx, zview path, sl loc)
{
  auto id{lo_import(raw_conn(tx), path.c_str())};
  if (id == 0)
    throw failure{
      std::format(
        "Could not import '{}' as a binary large object: {}", to_string(path),
        errmsg(tx)),
      loc};
  return id;
}


pqxx::oid pqxx::blob::from_file(dbtransaction &tx, zview path, oid id, sl loc)
{
  auto actual_id{lo_import_with_oid(raw_conn(tx), path.c_str(), id)};
  if (actual_id == 0)
    throw failure{
      std::format(
        "Could not import '{}' as binary large object {}: {}", to_string(path),
        id, errmsg(tx)),
      loc};
  return actual_id;
}


void pqxx::blob::to_file(dbtransaction &tx, oid id, zview path, sl loc)
{
  if (lo_export(raw_conn(tx), id, path.c_str()) < 0)
    throw failure{
      std::format(
        "Could not export binary large object {} to file '{}': {}", id,
        to_string(path), errmsg(tx)),
      loc};
}
