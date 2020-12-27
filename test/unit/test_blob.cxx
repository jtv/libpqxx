#include <pqxx/blob>
#include <pqxx/transaction>

#include "../test_helpers.hxx"
#include "../test_types.hxx"


namespace
{
void test_blob_is_useless_by_default()
{
  pqxx::blob b{};
  std::basic_string<std::byte> buf;
  PQXX_CHECK_THROWS(
    b.read(buf, 1), pqxx::usage_error,
    "Read on default-constructed blob did not throw failure.");
  PQXX_CHECK_THROWS(
    b.write(buf), pqxx::usage_error,
    "Write on default-constructed blob did not throw failure.");
}


void test_blob_create_makes_empty_blob()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::create(tx)};
  auto b{pqxx::blob::open_r(tx, id)};
  b.seek_end(0);
  PQXX_CHECK_EQUAL(b.tell(), 0, "New blob is not empty.");
}


void test_blobs_are_transactional()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::create(tx)};
  tx.abort();
  pqxx::work tx2{conn};
  PQXX_CHECK_THROWS(
    pqxx::ignore_unused(pqxx::blob::open_r(tx2, id)), pqxx::failure,
    "Blob from aborted transaction still exists.");
}


void test_blob_remove_removes_blob()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::create(tx)};
  pqxx::blob::remove(tx, id);
  PQXX_CHECK_THROWS(
    pqxx::ignore_unused(pqxx::blob::open_r(tx, id)), pqxx::failure,
    "Attempt to open blob after removing should have failed.");
}


void test_blob_remove_is_not_idempotent()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::create(tx)};
  pqxx::blob::remove(tx, id);
  PQXX_CHECK_THROWS(
    pqxx::blob::remove(tx, id), pqxx::failure,
    "Redundant remove() did not throw failure.");
}


void test_blob_checks_open_mode()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::create(tx)};
  pqxx::blob b_r{pqxx::blob::open_r(tx, id)};
  pqxx::blob b_w{pqxx::blob::open_w(tx, id)};
  pqxx::blob b_rw{pqxx::blob::open_rw(tx, id)};

  std::basic_string<std::byte> buf{std::byte{3}, std::byte{2}, std::byte{1}};

  // These are all allowed:
  b_w.write(buf);
  b_r.read(buf, 3);
  b_rw.seek_end(0);
  b_rw.write(buf);
  b_rw.seek_abs(0);
  b_rw.read(buf, 6);

  // These are not:
  PQXX_CHECK_THROWS(
    b_r.write(buf), pqxx::failure, "Read-only blob did not stop write.");
  PQXX_CHECK_THROWS(
    b_w.read(buf, 10), pqxx::failure, "Write-only blob did not stop read.");
}


void test_blob_supports_move()
{
  std::basic_string<std::byte> buf;
  buf.push_back(std::byte{'x'});

  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::create(tx)};
  pqxx::blob b1{pqxx::blob::open_rw(tx, id)};
  b1.write(buf);

  pqxx::blob b2{std::move(b1)};
  b2.seek_abs(0);
  b2.read(buf, 1u);

  PQXX_CHECK_THROWS(
    b1.read(buf, 1u), pqxx::usage_error,
    "Blob still works after move construction.");

  b1 = std::move(b2);
  b1.read(buf, 1u);

  PQXX_CHECK_THROWS(
    b2.read(buf, 1u), pqxx::usage_error,
    "Blob still works after move assignment.");
}


void test_blob_read_reads_data()
{
  std::basic_string<std::byte> const data{
    std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::oid id{pqxx::blob::from_buf(tx, data)};

  std::basic_string<std::byte> buf;
  auto b{pqxx::blob::open_rw(tx, id)};
  b.read(buf, 2);
  PQXX_CHECK_EQUAL(
    buf, (std::basic_string<std::byte>{std::byte{'a'}, std::byte{'b'}}),
    "Read back the wrong data.");
  b.read(buf, 2);
  PQXX_CHECK_EQUAL(
    buf, (std::basic_string<std::byte>{std::byte{'c'}}),
    "Continued read produced wrong data.");
  b.read(buf, 2);
  PQXX_CHECK_EQUAL(
    buf, (std::basic_string<std::byte>{}), "Read past end produced data.");
}


void test_blob_write_appends_at_insertion_point()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  auto id{pqxx::blob::create(tx)};

  auto b{pqxx::blob::open_rw(tx, id)};
  b.write(std::basic_string<std::byte>{std::byte{'z'}});
  b.write(std::basic_string<std::byte>{std::byte{'a'}});

  std::basic_string<std::byte> buf;
  b.read(buf, 5);
  PQXX_CHECK_EQUAL(
    buf, (std::basic_string<std::byte>{}), "Found data at the end.");
  b.seek_abs(0);
  b.read(buf, 5);
  PQXX_CHECK_EQUAL(
    buf, (std::basic_string<std::byte>{std::byte{'z'}, std::byte{'a'}}),
    "Consecutive writes did not append correctly.");

  b.write(std::basic_string<std::byte>{std::byte{'x'}});
  // Blob now contains "zax".  That's not we wanted...  Rewind and rewrite.
  b.seek_abs(1);
  b.write(std::basic_string<std::byte>{std::byte{'y'}});
  b.seek_abs(0);
  b.read(buf, 5);
  PQXX_CHECK_EQUAL(
    buf,
    (std::basic_string<std::byte>{
      std::byte{'z'}, std::byte{'y'}, std::byte{'x'}}),
    "Rewriting in the middle did not work right.");
}


void test_blob_resize_shortens_to_desired_length()
{
  std::basic_string<std::byte> const data{
    std::byte{'w'}, std::byte{'o'}, std::byte{'r'}, std::byte{'k'}};

  pqxx::connection conn;
  pqxx::work tx{conn};
  auto id{pqxx::blob::from_buf(tx, data)};

  pqxx::blob::open_w(tx, id).resize(2);
  std::basic_string<std::byte> buf;
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(
    buf, (std::basic_string<std::byte>{std::byte{'w'}, std::byte{'o'}}),
    "Truncate did not shorten correctly.");
}


void test_blob_resize_extends_to_desired_length()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  auto id{
    pqxx::blob::from_buf(tx, std::basic_string<std::byte>{std::byte{100}})};
  pqxx::blob::open_w(tx, id).resize(3);
  std::basic_string<std::byte> buf;
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(
    buf,
    (std::basic_string<std::byte>{std::byte{100}, std::byte{0}, std::byte{0}}),
    "Resize did not zero-extend correctly.");
}


void test_blob_tell_tracks_position()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  auto id{pqxx::blob::create(tx)};
  auto b{pqxx::blob::open_rw(tx, id)};

  PQXX_CHECK_EQUAL(
    b.tell(), 0, "Empty blob started out in non-zero position.");
  b.write(std::basic_string<std::byte>{std::byte{'e'}, std::byte{'f'}});
  PQXX_CHECK_EQUAL(
    b.tell(), 2, "Empty blob started out in non-zero position.");
  b.seek_abs(1);
  PQXX_CHECK_EQUAL(b.tell(), 1, "tell() did not track seek.");
}


void test_blob_seek_sets_positions()
{
  std::basic_string<std::byte> data{
    std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
    std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}};
  pqxx::connection conn;
  pqxx::work tx{conn};
  auto id{pqxx::blob::from_buf(tx, data)};
  auto b{pqxx::blob::open_r(tx, id)};

  std::basic_string<std::byte> buf;
  b.seek_rel(3);
  b.read(buf, 1u);
  PQXX_CHECK_EQUAL(
    buf[0], std::byte{3},
    "seek_rel() from beginning did not take us to the right position.");

  b.seek_abs(2);
  b.read(buf, 1u);
  PQXX_CHECK_EQUAL(
    buf[0], std::byte{2}, "seek_abs() did not take us to the right position.");

  b.seek_end(-2);
  b.read(buf, 1u);
  PQXX_CHECK_EQUAL(
    buf[0], std::byte{8}, "seek_end() did not take us to the right position.");
}


void test_blob_from_buf_interoperates_with_to_buf()
{
  std::basic_string<std::byte> const data{std::byte{'h'}, std::byte{'i'}};
  std::basic_string<std::byte> buf;
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::blob::to_buf(tx, pqxx::blob::from_buf(tx, data), buf, 10);
  PQXX_CHECK_EQUAL(buf, data, "from_buf()/to_buf() roundtrip did not work.");
}


void test_blob_append_from_buf_appends()
{
  std::basic_string<std::byte> const data{std::byte{'h'}, std::byte{'o'}};
  pqxx::connection conn;
  pqxx::work tx{conn};
  auto id{pqxx::blob::create(tx)};
  pqxx::blob::append_from_buf(tx, data, id);
  pqxx::blob::append_from_buf(tx, data, id);
  std::basic_string<std::byte> buf;
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(buf, data + data, "append_from_buf() wrote wrong data?");
}


void test_blob_from_file_creates_blob_from_file_contents()
{
  // XXX:
}


void test_blob_from_file_with_oid_writes_blob()
{
  // XXX:
}


PQXX_REGISTER_TEST(test_blob_is_useless_by_default);
PQXX_REGISTER_TEST(test_blob_create_makes_empty_blob);
PQXX_REGISTER_TEST(test_blobs_are_transactional);
PQXX_REGISTER_TEST(test_blob_remove_removes_blob);
PQXX_REGISTER_TEST(test_blob_remove_is_not_idempotent);
PQXX_REGISTER_TEST(test_blob_checks_open_mode);
PQXX_REGISTER_TEST(test_blob_supports_move);
PQXX_REGISTER_TEST(test_blob_read_reads_data);
PQXX_REGISTER_TEST(test_blob_write_appends_at_insertion_point);
PQXX_REGISTER_TEST(test_blob_resize_shortens_to_desired_length);
PQXX_REGISTER_TEST(test_blob_resize_extends_to_desired_length);
PQXX_REGISTER_TEST(test_blob_tell_tracks_position);
PQXX_REGISTER_TEST(test_blob_seek_sets_positions);
PQXX_REGISTER_TEST(test_blob_from_buf_interoperates_with_to_buf);
PQXX_REGISTER_TEST(test_blob_append_from_buf_appends);
PQXX_REGISTER_TEST(test_blob_from_file_creates_blob_from_file_contents);
PQXX_REGISTER_TEST(test_blob_from_file_with_oid_writes_blob);
} // namespace
