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
 * query or set the current reading/writing position, and so on.
 *
 * These large objects live in their own storage on the server, indexed by an
 * integer object identifier ("oid").
 *
 * Two @c blob objects may refer to the same actual large object in the
 * database at the same time.  Each will have its own reading/writing position,
 * but writes to the one will of course affect what the other sees.
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

  // XXX: Chunk 64-bit reads.
  /// Read up to @c size bytes of the object into @c buf.  Return bytes read.
  /** Uses a buffer that you provide, so that you can (if this suits you)
   * allocate it once and then re-use it multiple times.  This is more
   * efficient than creating and returning a new buffer every time.
   */
  void read(std::basic_string<std::byte> &buf, std::size_t size);

  // XXX: Chunk 64-bit writes.
  /// Write @c data to large object, at the current position.
  /** If the writing position is at the end of the object, this will append
   * @c data to the object's contents and move the writing position so that
   * it's still at the end.
   *
   * If the writing position was not at the end, writing will overwrite the
   * prior data, but it will not remove data that follows the part where you
   * wrote your new data.
   *
   * @warning This is a big difference from writing to a file.  You can
   * overwrite some data in a large object, but this does not truncate the
   * data that was already there.  For example, if the object contained binary
   * data "abc", and you write "12" at the starting position, the object will
   * contain "12c".
   */
  void write(std::basic_string_view<std::byte> data);

  /// Resize large object to @c size bytes.
  /** If the blob is more than @c size bytes long, this removes the end so as
   * to make the blob the desired length.
   *
   * If the blob is less than @c size bytes long, it adds enough zero bytes to
   * make it the desired length.
   */
  void resize(std::int64_t size = 0);

  /// Return the current reading/writing position in the large object.
  [[nodiscard]] std::int64_t tell() const;

  // XXX: Test.
  /// Set the current reading/writing position to an absolute offset.
  std::int64_t seek_abs(std::int64_t offset);
  // XXX: Test.
  /// Move the current reading/writing position forwards by an offset.
  std::int64_t seek_rel(std::int64_t offset);
  // XXX: Test.
  /// Set the current position to an offset relative to the end of the blob.
  std::int64_t seek_end(std::int64_t offset);

  // XXX: Test.
  /// Create a binary large object containing given @c data.
  /** You may optionally specify an oid for the new object.  If you do, and an
   * object with that oid already exists, creation will fail.
   */
  static oid from_buf(
    dbtransaction &tx, std::basic_string_view<std::byte> data, oid id = 0);

  // XXX: Test.
  static oid from_file(dbtransaction &, char const path[]);
  // XXX: Test.
  static oid from_file(dbtransaction &, char const path[], oid);
  // XXX: Test.
  static void to_buf(
    dbtransaction &, oid, std::basic_string<std::byte> &,
    std::int64_t max_size);
  // XXX: Test.
  static void to_file(dbtransaction &, oid, char const path[]);

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
