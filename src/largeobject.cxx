/** Implementation of the Large Objects interface.
 *
 * Allows direct access to large objects, as well as though I/O streams.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <algorithm>
#include <cerrno>
#include <stdexcept>

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/largeobject"

#include "pqxx/internal/gates/connection-largeobject.hxx"


namespace
{
constexpr inline int std_mode_to_pq_mode(std::ios::openmode mode)
{
  /// Mode bits, copied from libpq-fs.h so that we no longer need that header.
  constexpr int INV_WRITE{0x00020000}, INV_READ{0x00040000};

  return ((mode & std::ios::in) ? INV_READ : 0) |
         ((mode & std::ios::out) ? INV_WRITE : 0);
}


constexpr int std_dir_to_pq_dir(std::ios::seekdir dir) noexcept
{
  if constexpr (
    static_cast<int>(std::ios::beg) == int(SEEK_SET) and
    static_cast<int>(std::ios::cur) == int(SEEK_CUR) and
    static_cast<int>(std::ios::end) == int(SEEK_END))
  {
    // Easy optimisation: they're the same constants.  This is actually the
    // case for the gcc I'm using.
    return dir;
  }
  else
    switch (dir)
    {
    case std::ios::beg: return SEEK_SET; break;
    case std::ios::cur: return SEEK_CUR; break;
    case std::ios::end: return SEEK_END; break;

    // Shouldn't happen, but may silence compiler warning.
    default: return dir; break;
    }
}
} // namespace


pqxx::largeobject::largeobject(dbtransaction &t)
{
  // (Mode is ignored as of postgres 8.1.)
  m_id = lo_creat(raw_connection(t), 0);
  if (m_id == oid_none)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Could not create large object: " + reason(t.conn(), err)};
  }
}


pqxx::largeobject::largeobject(dbtransaction &t, std::string_view file)
{
  m_id = lo_import(raw_connection(t), file.data());
  if (m_id == oid_none)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{
      "Could not import file '" + std::string{file} +
      "' to large object: " + reason(t.conn(), err)};
  }
}


pqxx::largeobject::largeobject(largeobjectaccess const &o) noexcept :
        m_id{o.id()}
{}


void pqxx::largeobject::to_file(dbtransaction &t, std::string_view file) const
{
  if (id() == oid_none)
    throw usage_error{"No object selected."};
  if (lo_export(raw_connection(t), id(), file.data()) == -1)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{
      "Could not export large object " + to_string(m_id) +
      " "
      "to file '" +
      std::string{file} + "': " + reason(t.conn(), err)};
  }
}


void pqxx::largeobject::remove(dbtransaction &t) const
{
  if (id() == oid_none)
    throw usage_error{"No object selected."};
  if (lo_unlink(raw_connection(t), id()) == -1)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{
      "Could not delete large object " + to_string(m_id) + ": " +
      reason(t.conn(), err)};
  }
}


pqxx::internal::pq::PGconn *
pqxx::largeobject::raw_connection(dbtransaction const &t)
{
  return pqxx::internal::gate::connection_largeobject{t.conn()}
    .raw_connection();
}


std::string pqxx::largeobject::reason(connection const &c, int err) const
{
  if (err == ENOMEM)
    return "Out of memory";
  return pqxx::internal::gate::const_connection_largeobject{c}.error_message();
}


pqxx::largeobjectaccess::largeobjectaccess(dbtransaction &t, openmode mode) :
        largeobject{t}, m_trans{t}
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(
  dbtransaction &t, oid o, openmode mode) :
        largeobject{o}, m_trans{t}
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(
  dbtransaction &t, largeobject o, openmode mode) :
        largeobject{o}, m_trans{t}
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(
  dbtransaction &t, std::string_view file, openmode mode) :
        largeobject{t, file}, m_trans{t}
{
  open(mode);
}


pqxx::largeobjectaccess::size_type
pqxx::largeobjectaccess::seek(size_type dest, seekdir dir)
{
  auto const res{cseek(dest, dir)};
  if (res == -1)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    if (id() == oid_none)
      throw usage_error{"No object selected."};
    throw failure{"Error seeking in large object: " + reason(err)};
  }

  return res;
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cseek(off_type dest, seekdir dir) noexcept
{
  return lo_lseek64(raw_connection(), m_fd, dest, std_dir_to_pq_dir(dir));
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cwrite(char const buf[], std::size_t len) noexcept
{
  return std::max(lo_write(raw_connection(), m_fd, buf, len), -1);
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cread(char buf[], std::size_t len) noexcept
{
  return std::max(lo_read(raw_connection(), m_fd, buf, len), -1);
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::ctell() const noexcept
{
  return lo_tell64(raw_connection(), m_fd);
}


void pqxx::largeobjectaccess::write(char const buf[], std::size_t len)
{
  if (id() == oid_none)
    throw usage_error{"No object selected."};
  if (auto const bytes{cwrite(buf, len)}; bytes < static_cast<off_type>(len))
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    if (bytes < 0)
      throw failure{
        "Error writing to large object #" + to_string(id()) + ": " +
        reason(err)};
    if (bytes == 0)
      throw failure{
        "Could not write to large object #" + to_string(id()) + ": " +
        reason(err)};

    throw failure{
      "Wanted to write " + to_string(len) + " bytes to large object #" +
      to_string(id()) +
      "; "
      "could only write " +
      to_string(bytes)};
  }
}


pqxx::largeobjectaccess::size_type
pqxx::largeobjectaccess::read(char buf[], std::size_t len)
{
  if (id() == oid_none)
    throw usage_error{"No object selected."};
  auto const bytes{cread(buf, len)};
  if (bytes < 0)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{
      "Error reading from large object #" + to_string(id()) + ": " +
      reason(err)};
  }
  return bytes;
}


void pqxx::largeobjectaccess::open(openmode mode)
{
  if (id() == oid_none)
    throw usage_error{"No object selected."};
  m_fd = lo_open(raw_connection(), id(), std_mode_to_pq_mode(mode));
  if (m_fd < 0)
  {
    int const err{errno};
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{
      "Could not open large object " + to_string(id()) + ": " + reason(err)};
  }
}


void pqxx::largeobjectaccess::close() noexcept
{
  if (m_fd >= 0)
    lo_close(raw_connection(), m_fd);
}


pqxx::largeobjectaccess::size_type pqxx::largeobjectaccess::tell() const
{
  auto const res{ctell()};
  if (res == -1)
    throw failure{reason(errno)};
  return res;
}


std::string pqxx::largeobjectaccess::reason(int err) const
{
  if (m_fd == -1)
    return "No object opened.";
  return largeobject::reason(m_trans.conn(), err);
}


void pqxx::largeobjectaccess::process_notice(std::string const &s) noexcept
{
  m_trans.process_notice(s);
}
