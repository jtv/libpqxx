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
constexpr inline int StdModeToPQMode(std::ios::openmode mode)
{
  /// Mode bits, copied from libpq-fs.h so that we no longer need that header.
  constexpr int INV_WRITE = 0x00020000, INV_READ = 0x00040000;

  return ((mode & std::ios::in) ? INV_READ : 0) |
         ((mode & std::ios::out) ? INV_WRITE : 0);
}


constexpr int StdDirToPQDir(std::ios::seekdir dir) noexcept
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


pqxx::largeobject::largeobject(dbtransaction &T) : m_id{}
{
  // (Mode is ignored as of postgres 8.1.)
  m_id = lo_creat(raw_connection(T), 0);
  if (m_id == oid_none)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Could not create large object: " + reason(T.conn(), err)};
  }
}


pqxx::largeobject::largeobject(dbtransaction &T, std::string_view File) :
        m_id{}
{
  m_id = lo_import(raw_connection(T), File.data());
  if (m_id == oid_none)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Could not import file '" + std::string{File} +
                  "' to large object: " + reason(T.conn(), err)};
  }
}


pqxx::largeobject::largeobject(const largeobjectaccess &O) noexcept :
        m_id{O.id()}
{}


void pqxx::largeobject::to_file(dbtransaction &T, std::string_view File) const
{
  if (lo_export(raw_connection(T), id(), File.data()) == -1)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Could not export large object " + to_string(m_id) +
                  " "
                  "to file '" +
                  std::string{File} + "': " + reason(T.conn(), err)};
  }
}


void pqxx::largeobject::remove(dbtransaction &T) const
{
  if (lo_unlink(raw_connection(T), id()) == -1)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Could not delete large object " + to_string(m_id) + ": " +
                  reason(T.conn(), err)};
  }
}


pqxx::internal::pq::PGconn *
pqxx::largeobject::raw_connection(const dbtransaction &T)
{
  return pqxx::internal::gate::connection_largeobject{T.conn()}
    .raw_connection();
}


std::string pqxx::largeobject::reason(const connection &c, int err) const
{
  if (err == ENOMEM)
    return "Out of memory";
  if (id() == oid_none)
    return "No object selected";
  return pqxx::internal::gate::const_connection_largeobject{c}.error_message();
}


pqxx::largeobjectaccess::largeobjectaccess(dbtransaction &T, openmode mode) :
        largeobject{T},
        m_trans{T}
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(
  dbtransaction &T, oid O, openmode mode) :
        largeobject{O},
        m_trans{T}
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(
  dbtransaction &T, largeobject O, openmode mode) :
        largeobject{O},
        m_trans{T}
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(
  dbtransaction &T, std::string_view File, openmode mode) :
        largeobject{T, File},
        m_trans{T}
{
  open(mode);
}


pqxx::largeobjectaccess::size_type
pqxx::largeobjectaccess::seek(size_type dest, seekdir dir)
{
  const auto Result = cseek(dest, dir);
  if (Result == -1)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Error seeking in large object: " + reason(err)};
  }

  return Result;
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cseek(off_type dest, seekdir dir) noexcept
{
  return lo_lseek64(raw_connection(), m_fd, dest, StdDirToPQDir(dir));
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cwrite(const char Buf[], size_t Len) noexcept
{
  return std::max(
    lo_write(raw_connection(), m_fd, const_cast<char *>(Buf), size_t(Len)),
    -1);
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cread(char Buf[], size_t Len) noexcept
{
  return std::max(lo_read(raw_connection(), m_fd, Buf, size_t(Len)), -1);
}


pqxx::largeobjectaccess::pos_type pqxx::largeobjectaccess::ctell() const
  noexcept
{
  return lo_tell64(raw_connection(), m_fd);
}


void pqxx::largeobjectaccess::write(const char Buf[], size_t Len)
{
  if (const auto Bytes = cwrite(Buf, Len); Bytes < static_cast<off_type>(Len))
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    if (Bytes < 0)
      throw failure{"Error writing to large object #" + to_string(id()) +
                    ": " + reason(err)};
    if (Bytes == 0)
      throw failure{"Could not write to large object #" + to_string(id()) +
                    ": " + reason(err)};

    throw failure{"Wanted to write " + to_string(Len) +
                  " bytes to large object #" + to_string(id()) +
                  "; "
                  "could only write " +
                  to_string(Bytes)};
  }
}


pqxx::largeobjectaccess::size_type
pqxx::largeobjectaccess::read(char Buf[], size_t Len)
{
  const auto Bytes = cread(Buf, Len);
  if (Bytes < 0)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Error reading from large object #" + to_string(id()) +
                  ": " + reason(err)};
  }
  return Bytes;
}


void pqxx::largeobjectaccess::open(openmode mode)
{
  m_fd = lo_open(raw_connection(), id(), StdModeToPQMode(mode));
  if (m_fd < 0)
  {
    const int err = errno;
    if (err == ENOMEM)
      throw std::bad_alloc{};
    throw failure{"Could not open large object " + to_string(id()) + ": " +
                  reason(err)};
  }
}


void pqxx::largeobjectaccess::close() noexcept
{
  if (m_fd >= 0)
    lo_close(raw_connection(), m_fd);
}


pqxx::largeobjectaccess::size_type pqxx::largeobjectaccess::tell() const
{
  const size_type res = ctell();
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


void pqxx::largeobjectaccess::process_notice(const std::string &s) noexcept
{
  m_trans.process_notice(s);
}
