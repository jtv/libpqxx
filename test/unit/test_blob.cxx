#include <pqxx/blob>
#include <pqxx/transaction>

#include "../test_helpers.hxx"


namespace
{
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

  std::basic_string<std::byte> buf;
  buf.push_back(std::byte{3});
  buf.push_back(std::byte{2});
  buf.push_back(std::byte{1});

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
    "Blob still works after move-construction.");

  b1 = std::move(b2);
  b1.read(buf, 1u);

  PQXX_CHECK_THROWS(
    b2.read(buf, 1u), pqxx::usage_error,
    "Blob still works after mov-assignment.");
}


PQXX_REGISTER_TEST(test_blob_create_makes_empty_blob);
PQXX_REGISTER_TEST(test_blobs_are_transactional);
PQXX_REGISTER_TEST(test_blob_remove_removes_blob);
PQXX_REGISTER_TEST(test_blob_remove_is_not_idempotent);
PQXX_REGISTER_TEST(test_blob_checks_open_mode);
PQXX_REGISTER_TEST(test_blob_supports_move);
} // namespace
