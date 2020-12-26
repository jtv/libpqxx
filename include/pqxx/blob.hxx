/* Binary Large Objects interface.
 *
 * Read or write large objects, stored in their own storage on the server.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/largeobject instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_BLOB
#define PQXX_H_BLOB

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <cstdint>

#include "pqxx/dbtransaction.hxx"


namespace pqxx
{
/** Binary large object.
 *
 * This is how you store data that may be too large for the @c BYTEA type.
 * Access operations are similar to those for a file: you can read, write,
 *
 * These large objects live in their own storage on the server, indexed by an
 * object identifier ("oid").
 */
class PQXX_LIBEXPORT blob
{
public:
  /// Create a new, empty large object.
  /** You may optionally specify an oid for the new blob.  If you do, then
   * the new object will have that oid -- or creation will fail if there
   * already is an object with that oid.
   */
  [[nodiscard]] static oid create(dbtransaction &, oid = 0);

  /// Delete a large object, or fail if it does not exist.
  static void remove(dbtransaction &, oid);

  /// Open blob for reading.  Any attempt to write to it will fail.
  [[nodiscard]] static blob open_r(dbtransaction &, oid);
  // Open blob for writing.  Any attempt to read from it will fail.
  [[nodiscard]] static blob open_w(dbtransaction &, oid);
  // Open blob for reading and/or writing.
  [[nodiscard]] static blob open_rw(dbtransaction &, oid);

  blob(blob &&);
  blob &operator=(blob &&);

  blob() = delete;
  blob(blob const &) = delete;
  blob &operator=(blob const &) = delete;
  ~blob();

  // XXX: Test.
  void read(std::basic_string<std::byte> &, std::size_t);
  // XXX: Test.
  void write(std::basic_string_view<std::byte>);
  // XXX: Test.
  void truncate(std::int64_t size = 0);

  // XXX: Test.
  [[nodiscard]] std::int64_t tell() const;
  // XXX: Test.
  std::int64_t seek_abs(std::int64_t);
  // XXX: Test.
  std::int64_t seek_rel(std::int64_t);
  // XXX: Test.
  std::int64_t seek_end(std::int64_t);

  // XXX: from_data() / to_data()

  // XXX: Test.
  static oid from_file(dbtransaction &, char const path[]);
  // XXX: Test.
  static oid from_file(dbtransaction &, char const path[], oid);
  // XXX: Test.
  static void to_file(dbtransaction &, char const path[], oid);

  // XXX: Test.
  void close();

private:
  PQXX_PRIVATE blob(connection &conn, int fd) noexcept :
          m_conn{&conn}, m_fd{fd}
  {}
  static PQXX_PRIVATE blob open_internal(dbtransaction &, oid, int);
  static PQXX_PRIVATE pqxx::internal::pq::PGconn *
  raw_conn(pqxx::connection *) noexcept;
  static PQXX_PRIVATE pqxx::internal::pq::PGconn *
  raw_conn(pqxx::dbtransaction const &) noexcept;
  static PQXX_PRIVATE std::string errmsg(connection const *);
  static PQXX_PRIVATE std::string errmsg(dbtransaction const &tx)
  {
    return errmsg(&tx.conn());
  }
  PQXX_PRIVATE std::string errmsg() const { return errmsg(m_conn); }
  PQXX_PRIVATE std::int64_t seek(std::int64_t offset, int whence);

  connection *m_conn;
  int m_fd;
};
} // namespace pqxx
#endif
