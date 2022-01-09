/* Binary Large Objects interface.
 *
 * Read or write large objects, stored in their own storage on the server.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/largeobject instead.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
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

  /// You can default-construct a blob, but it won't do anything useful.
  /** Most operations on a default-constructed blob will throw @c usage_error.
   */
  blob() = default;

  /// You can move a blob, but not copy it.  The original becomes unusable.
  blob(blob &&);
  /// You can move a blob, but not copy it.  The original becomes unusable.
  blob &operator=(blob &&);

  blob(blob const &) = delete;
  blob &operator=(blob const &) = delete;
  ~blob();

  /// Maximum number of bytes that can be read or written at a time.
  /** The underlying protocol only supports reads and writes up to 2 GB
   * exclusive.
   *
   * If you need to read or write more data to or from a binary large object,
   * you'll have to break it up into chunks.
   */
  static constexpr std::size_t chunk_limit = 0x7fffffff;

  /// Read up to @c size bytes of the object into @c buf.  Return bytes read.
  /** Uses a buffer that you provide, so that you can (if this suits you)
   * allocate it once and then re-use it multiple times.  This is more
   * efficient than creating and returning a new buffer every time.
   *
   * @warning The underlying protocol only supports reads up to 2GB at a time.
   * If you need to read more, try making repeated calls to @c append_to_buf.
   */
  std::size_t read(std::basic_string<std::byte> &buf, std::size_t size);

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
   *
   * @warning The underlying protocol only supports writes up to 2 GB at a
   * time.  If you need to write more, try making repeated calls to
   * @c append_from_buf.
   */
  void write(std::basic_string_view<std::byte> data);

  /// Resize large object to @c size bytes.
  /** If the blob is more than @c size bytes long, this removes the end so as
   * to make the blob the desired length.
   *
   * If the blob is less than @c size bytes long, it adds enough zero bytes to
   * make it the desired length.
   */
  void resize(std::int64_t size);

  /// Return the current reading/writing position in the large object.
  [[nodiscard]] std::int64_t tell() const;

  /// Set the current reading/writing position to an absolute offset.
  std::int64_t seek_abs(std::int64_t offset = 0);
  /// Move the current reading/writing position forwards by an offset.
  std::int64_t seek_rel(std::int64_t offset = 0);
  /// Set the current position to an offset relative to the end of the blob.
  std::int64_t seek_end(std::int64_t offset = 0);

  /// Create a binary large object containing given @c data.
  /** You may optionally specify an oid for the new object.  If you do, and an
   * object with that oid already exists, creation will fail.
   */
  static oid from_buf(
    dbtransaction &tx, std::basic_string_view<std::byte> data, oid id = 0);

  /// Append @c data to binary large object.
  /** The underlying protocol only supports appending blocks up to 2 GB.
   */
  static void append_from_buf(
    dbtransaction &tx, std::basic_string_view<std::byte> data, oid id);

  /// Read client-side file and store it server-side as a binary large object.
  static oid from_file(dbtransaction &, char const path[]);

  /// Read client-side file and store it server-side as a binary large object.
  /** In this version, you specify the binary large object's oid.  If that oid
   * is already in use, the operation will fail.
   */
  static oid from_file(dbtransaction &, char const path[], oid);

  /// Convenience function: Read up to @c max_size bytes from blob with @c id.
  /** You could easily do this yourself using the @c open_r and @c read
   * functions, but it can save you a bit of code to do it this way.
   */
  static void to_buf(
    dbtransaction &, oid, std::basic_string<std::byte> &,
    std::size_t max_size);

  /// Read part of the binary large object with @c id, and append it to @c buf.
  /** Use this to break up a large read from one binary large object into one
   * massive buffer.  Just keep calling this function until it returns zero.
   *
   * The @c offset is how far into the large object your desired chunk is, and
   * @c append_max says how much to try and read in one go.
   */
  static std::size_t append_to_buf(
    dbtransaction &tx, oid id, std::int64_t offset,
    std::basic_string<std::byte> &buf, std::size_t append_max);

  /// Write a binary large object's contents to a client-side file.
  static void to_file(dbtransaction &, oid, char const path[]);

  /// Close this blob (but leave the actual binary large object on the server).
  /** Resets the blob to a useless state similar to one that was
   * default-constructed.
   *
   * You don't have to call this.  The destructor will do it for you.  However
   * in the unlikely event that closing the object should fail, the destructor
   * can't throw an exception.  The @c close() function can.
   */
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

  connection *m_conn = nullptr;
  int m_fd = -1;
};
} // namespace pqxx
#endif
