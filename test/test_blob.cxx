#include <cerrno>
#include <cstdio>

#include <pqxx/blob>
#include <pqxx/transaction>

#include "helpers.hxx"
#include "sample_types.hxx"


namespace
{
void test_blob_is_useless_by_default(pqxx::test::context &)
{
  pqxx::blob b{};
  pqxx::bytes buf;
  PQXX_CHECK_THROWS(b.read(buf, 1), pqxx::usage_error);
  PQXX_CHECK_THROWS(b.write(buf), pqxx::usage_error);
}


void test_blob_create_makes_empty_blob(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid const id{pqxx::blob::create(tx)};
  auto b{pqxx::blob::open_r(tx, id)};
  b.seek_end(0);
  PQXX_CHECK_EQUAL(b.tell(), 0);
}


void test_blob_create_with_oid_requires_oid_be_free(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::create(tx)};

  PQXX_CHECK_THROWS(std::ignore = pqxx::blob::create(tx, id), pqxx::failure);
}


void test_blob_create_with_oid_obeys_oid(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::create(tx)};
  pqxx::blob::remove(tx, id);

  auto actual_id{pqxx::blob::create(tx, id)};
  pqxx::blob::remove(tx, actual_id);
  PQXX_CHECK_EQUAL(actual_id, id);
}


void test_blobs_are_transactional(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid id{pqxx::blob::create(tx)};
  tx.abort();
  pqxx::work tx2{cx};
  PQXX_CHECK_THROWS(std::ignore = pqxx::blob::open_r(tx2, id), pqxx::failure);
}


void test_blob_remove_removes_blob(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid id{pqxx::blob::create(tx)};
  pqxx::blob::remove(tx, id);
  PQXX_CHECK_THROWS(std::ignore = pqxx::blob::open_r(tx, id), pqxx::failure);

  PQXX_CHECK_THROWS(pqxx::blob::remove(tx, 0), pqxx::usage_error);
}


void test_blob_remove_is_not_idempotent(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid id{pqxx::blob::create(tx)};
  pqxx::blob::remove(tx, id);
  PQXX_CHECK_THROWS(pqxx::blob::remove(tx, id), pqxx::failure);
}


void test_blob_checks_open_mode(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid const id{pqxx::blob::create(tx)};
  pqxx::blob b_r{pqxx::blob::open_r(tx, id)};
  pqxx::blob b_w{pqxx::blob::open_w(tx, id)};
  pqxx::blob b_rw{pqxx::blob::open_rw(tx, id)};

  pqxx::bytes buf{std::byte{3}, std::byte{2}, std::byte{1}};

  // These are all allowed:
  b_w.write(buf);
  b_r.read(buf, 3);
  b_rw.seek_end(0);
  b_rw.write(buf);
  b_rw.seek_abs(0);
  b_rw.read(buf, 6);

  // These are not:
  PQXX_CHECK_THROWS(b_r.write(buf), pqxx::failure);
  PQXX_CHECK_THROWS(b_w.read(buf, 10), pqxx::failure);
}


/// Convert a byte to an unsigned int.
unsigned byte_as_num(std::byte b)
{
  return static_cast<unsigned char>(b);
}


void test_blob_supports_move(pqxx::test::context &)
{
  pqxx::bytes buf;
  buf.push_back(std::byte{'x'});

  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid const id{pqxx::blob::create(tx)};
  pqxx::blob b1{pqxx::blob::open_rw(tx, id)};
  b1.write(buf);

  pqxx::blob b2{std::move(b1)};
  b2.seek_abs(0);
  b2.read(buf, 1u);

  PQXX_CHECK_THROWS(b1.read(buf, 1u), pqxx::usage_error); // NOLINT

  b1 = std::move(b2);
  b1.read(buf, 1u);

  PQXX_CHECK_THROWS(b2.read(buf, 1u), pqxx::usage_error); // NOLINT

  std::array<std::byte, 1> const three{std::byte{0x03}}, four{std::byte{0x04}};
  auto const id3{pqxx::blob::from_buf(tx, three)},
    id4{pqxx::blob::from_buf(tx, four)};
  auto b3{pqxx::blob::open_rw(tx, id3)}, b4{pqxx::blob::open_rw(tx, id4)};
  b3 = std::move(b4);
  // The move invalidates b4.
  // NOLINTNEXTLINE
  PQXX_CHECK_THROWS(b4.seek_abs(0), pqxx::usage_error);
  b3.seek_abs(0);

  // Because of the assignment, b3 now refers to the blob with ID id4.
  std::array<std::byte, 2> buf2{};
  auto const out{b3.read(buf2)};
  PQXX_CHECK_EQUAL(std::size(out), 1u);
  PQXX_CHECK_EQUAL(byte_as_num(out[0]), byte_as_num(std::byte{0x04}));
}


void test_blob_read_reads_data(pqxx::test::context &)
{
  pqxx::bytes const data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid const id{pqxx::blob::from_buf(tx, data)};

  pqxx::bytes buf;
  auto b{pqxx::blob::open_rw(tx, id)};
  PQXX_CHECK_EQUAL(b.read(buf, 2), 2u);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{std::byte{'a'}, std::byte{'b'}}));
  PQXX_CHECK_EQUAL(b.read(buf, 2), 1u);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{std::byte{'c'}}));
  PQXX_CHECK_EQUAL(b.read(buf, 2), 0u);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{}));
}


void test_blob_read_reads_generic_data(pqxx::test::context &)
{
  std::array<std::byte, 3> const data{
    std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid const id{pqxx::blob::from_buf(tx, data)};

  pqxx::bytes buf;
  auto b{pqxx::blob::open_rw(tx, id)};
  PQXX_CHECK_EQUAL(b.read(buf, 2), 2u);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{std::byte{'a'}, std::byte{'b'}}));
  PQXX_CHECK_EQUAL(b.read(buf, 2), 1u);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{std::byte{'c'}}));
  PQXX_CHECK_EQUAL(b.read(buf, 2), 0u);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{}));
}


/// Cast a `char` or `std::byte` to `unsigned int`.
template<typename BYTE> inline unsigned byte_val(BYTE val)
{
  return unsigned(static_cast<unsigned char>(val));
}


void test_blob_read_span(pqxx::test::context &)
{
  pqxx::bytes const data{std::byte{'u'}, std::byte{'v'}, std::byte{'w'},
                         std::byte{'x'}, std::byte{'y'}, std::byte{'z'}};

  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::oid const id{pqxx::blob::from_buf(tx, data)};

  auto b{pqxx::blob::open_r(tx, id)};
  pqxx::bytes string_buf;
  string_buf.resize(2);

  std::span<std::byte> output;

  output = b.read(std::span<std::byte>{});
  PQXX_CHECK_EQUAL(std::size(output), 0u);
  output = b.read(string_buf);
  PQXX_CHECK_EQUAL(std::size(output), 2u);
  PQXX_CHECK_EQUAL(byte_val(output[0]), byte_val('u'));
  PQXX_CHECK_EQUAL(byte_val(output[1]), byte_val('v'));

  string_buf.resize(100);
  output = b.read(std::span<std::byte>{string_buf.data(), 1});
  PQXX_CHECK_EQUAL(std::size(output), 1u);
  PQXX_CHECK_EQUAL(byte_val(output[0]), byte_val('w'));

  std::vector<std::byte> vec_buf;
  vec_buf.resize(2);
  auto output2{b.read(vec_buf)};
  PQXX_CHECK_EQUAL(std::size(output2), 2u);
  PQXX_CHECK_EQUAL(byte_val(output2[0]), byte_val('x'));
  PQXX_CHECK_EQUAL(byte_val(output2[1]), byte_val('y'));

  vec_buf.resize(100);
  output2 = b.read(vec_buf);
  PQXX_CHECK_EQUAL(std::size(output2), 1u);
  PQXX_CHECK_EQUAL(byte_val(output2[0]), byte_val('z'));
}


void test_blob_reads_vector(pqxx::test::context &)
{
  char const content[]{"abcd"};
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(
    tx, pqxx::bytes_view{
          reinterpret_cast<std::byte const *>(content), std::size(content)})};
  std::vector<std::byte> buf;
  buf.resize(10);
  auto out{pqxx::blob::open_r(tx, id).read(buf)};
  PQXX_CHECK_EQUAL(std::size(out), std::size(content));
  PQXX_CHECK_EQUAL(byte_val(out[0]), byte_val('a'));
}


void test_blob_write_appends_at_insertion_point(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::create(tx)};

  auto b{pqxx::blob::open_rw(tx, id)};
  b.write(pqxx::bytes{std::byte{'z'}});
  b.write(pqxx::bytes{std::byte{'a'}});

  pqxx::bytes buf;
  b.read(buf, 5);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{}), "Found data at the end.");
  b.seek_abs(0);
  b.read(buf, 5);
  PQXX_CHECK_EQUAL(
    buf, (pqxx::bytes{std::byte{'z'}, std::byte{'a'}}),
    "Consecutive writes did not append correctly.");

  b.write(pqxx::bytes{std::byte{'x'}});
  // Blob now contains "zax".  That's not we wanted...  Rewind and rewrite.
  b.seek_abs(1);
  b.write(pqxx::bytes{std::byte{'y'}});
  b.seek_abs(0);
  b.read(buf, 5);
  PQXX_CHECK_EQUAL(
    buf, (pqxx::bytes{std::byte{'z'}, std::byte{'y'}, std::byte{'x'}}),
    "Rewriting in the middle did not work right.");
}


void test_blob_writes_span(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  constexpr char content[]{"gfbltk"};
  // C++23: Initialise as data{std::from_range_t, content}?
  pqxx::bytes data;
  for (char c : content) data.push_back(static_cast<std::byte>(c));

  auto id{pqxx::blob::create(tx)};
  auto b{pqxx::blob::open_rw(tx, id)};
  b.write(std::span<std::byte>{data.data() + 1, 3u});
  b.seek_abs(0);

  std::vector<std::byte> buf;
  buf.resize(4);
  auto out{b.read(std::span<std::byte>{buf.data(), 4u})};
  PQXX_CHECK_EQUAL(std::size(out), 3u);
  PQXX_CHECK_EQUAL(byte_val(out[0]), byte_val('f'));
  PQXX_CHECK_EQUAL(byte_val(out[2]), byte_val('l'));
}


void test_blob_resize_shortens_to_desired_length(pqxx::test::context &)
{
  pqxx::bytes const data{
    std::byte{'w'}, std::byte{'o'}, std::byte{'r'}, std::byte{'k'}};

  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(tx, data)};

  pqxx::blob::open_w(tx, id).resize(2);
  pqxx::bytes buf;
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(buf, (pqxx::bytes{std::byte{'w'}, std::byte{'o'}}));
}


void test_blob_resize_extends_to_desired_length(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(tx, pqxx::bytes{std::byte{100}})};
  pqxx::blob::open_w(tx, id).resize(3);
  pqxx::bytes buf;
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(
    buf, (pqxx::bytes{std::byte{100}, std::byte{0}, std::byte{0}}),
    "Resize did not zero-extend correctly.");
}


void test_blob_tell_tracks_position(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::create(tx)};
  auto b{pqxx::blob::open_rw(tx, id)};

  PQXX_CHECK_EQUAL(b.tell(), 0);
  b.write(pqxx::bytes{std::byte{'e'}, std::byte{'f'}});
  PQXX_CHECK_EQUAL(b.tell(), 2);
  b.seek_abs(1);
  PQXX_CHECK_EQUAL(b.tell(), 1);

  b.close();
  PQXX_CHECK_THROWS(b.seek_abs(0), pqxx::usage_error);
  PQXX_CHECK_THROWS(std::ignore = b.tell(), pqxx::usage_error);
}


void test_blob_seek_sets_positions(pqxx::test::context &)
{
  pqxx::bytes const data{
    std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
    std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}};
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(tx, data)};
  auto b{pqxx::blob::open_r(tx, id)};

  pqxx::bytes buf;
  b.seek_rel(3);
  b.read(buf, 1u);
  PQXX_CHECK_EQUAL(byte_val(buf.at(0)), byte_val(3));

  b.seek_abs(2);
  b.read(buf, 1u);
  PQXX_CHECK_EQUAL(byte_val(buf.at(0)), byte_val(2));

  b.seek_end(-2);
  b.read(buf, 1u);
  PQXX_CHECK_EQUAL(byte_val(buf.at(0)), byte_val(8));
}


void test_blob_from_buf_interoperates_with_to_buf(pqxx::test::context &)
{
  pqxx::bytes const data{std::byte{'h'}, std::byte{'i'}};
  pqxx::bytes buf;
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::blob::to_buf(tx, pqxx::blob::from_buf(tx, data), buf, 10);
  PQXX_CHECK_EQUAL(buf, data);
}


void test_blob_append_from_buf_appends(pqxx::test::context &)
{
  pqxx::bytes const data{std::byte{'h'}, std::byte{'o'}};
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::create(tx)};
  pqxx::blob::append_from_buf(tx, data, id);
  pqxx::blob::append_from_buf(tx, data, id);
  pqxx::bytes buf;
  pqxx::blob::to_buf(tx, id, buf, 10);

  auto expect{data};
  // Infer thinks the push_back() may invalidate e!?
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges
  expect.append_range(data);
#else
  for (auto const &e : data) expect.push_back(e);
#endif

  PQXX_CHECK_EQUAL(buf, expect);
}


void test_blob_generic_append_from_buf_appends(pqxx::test::context &)
{
  std::array<std::byte, 2> const data{std::byte{'h'}, std::byte{'o'}};
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::create(tx)};
  pqxx::blob::append_from_buf(tx, data, id);
  pqxx::blob::append_from_buf(tx, data, id);
  pqxx::bytes buf;
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(std::size(buf), 2 * std::size(data));
}


namespace
{
/// Wrap `std::fopen`.
/** This is just here to stop Visual Studio from advertising its own
 * alternative.
 */
std::unique_ptr<FILE, int (*)(FILE *)>
my_fopen(char const *path, char const *mode)
{
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
  std::array<char, 300> buffer{};
  std::unique_ptr<FILE, int (*)(FILE *)> handle{
    std::fopen(path, mode), std::fclose};
  if (not handle)
  {
    std::string const message{pqxx::internal::error_string(errno, buffer)};
    throw std::runtime_error{
      std::format("Failed to open '{}' ({}): {}.", path, mode, message)};
  }
  return handle;
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
}


void read_file(char const path[], std::size_t len, pqxx::bytes &buf)
{
  buf.resize(len);
  auto f{my_fopen(path, "rb")};
  PQXX_CHECK(f.get() != nullptr, std::format("Did not open file '{}'!", path));
  auto bytes{
    std::fread(reinterpret_cast<char *>(buf.data()), 1, len, f.get())};
  if (bytes == 0)
    throw std::runtime_error{"Error reading test file."};
  buf.resize(bytes);
}


void write_file(char const path[], pqxx::bytes_view data)
{
  try
  {
    auto f{my_fopen(path, "wb")};
    if (
      std::fwrite(
        reinterpret_cast<char const *>(data.data()), 1, std::size(data),
        f.get()) < std::size(data))
      throw std::runtime_error{"File write failed."};
    if (std::fflush(f.get()) != 0)
      throw std::runtime_error{"File flush failed."};
  }
  catch (const std::exception &)
  {
    std::ignore = std::remove(path);
    throw;
  }
}


/// Temporary file.
class TempFile
{
public:
  TempFile() = delete;
  TempFile(TempFile const &) = delete;
  TempFile(TempFile &&) = delete;
  TempFile &operator=(TempFile const &) = delete;
  TempFile &operator=(TempFile &&) = delete;

  /// Create (and later clean up) a file at path containing data.
  TempFile(char const path[], pqxx::bytes_view data) : m_path(path)
  {
    write_file(path, data);
  }

  ~TempFile() { std::ignore = std::remove(m_path.c_str()); }

private:
  std::string m_path;
};
} // namespace


void test_blob_from_file_creates_blob_from_file_contents(
  pqxx::test::context &tctx)
{
  std::string const temp_file = tctx.make_name("pqxx-blob");
  pqxx::bytes const data{std::byte{'4'}, std::byte{'2'}};

  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::bytes buf;

  pqxx::oid id{};
  {
    TempFile const f{temp_file.c_str(), data};
    id = pqxx::blob::from_file(tx, std::data(temp_file));
  }
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(buf, data);
}


void test_blob_from_file_with_oid_writes_blob(pqxx::test::context &tctx)
{
  pqxx::bytes const data{std::byte{'6'}, std::byte{'9'}};
  std::string const temp_file = tctx.make_name("pqxx-blob");
  pqxx::bytes buf;

  pqxx::connection cx;
  pqxx::work tx{cx};

  // Guarantee (more or less) that id is not in use.
  auto id{pqxx::blob::create(tx)};
  pqxx::blob::remove(tx, id);

  {
    TempFile const f{temp_file.c_str(), data};
    pqxx::blob::from_file(tx, std::data(temp_file), id);
  }
  pqxx::blob::to_buf(tx, id, buf, 10);
  PQXX_CHECK_EQUAL(buf, data);
}


void test_blob_append_to_buf_appends(pqxx::test::context &)
{
  pqxx::bytes const data{
    std::byte{'b'}, std::byte{'l'}, std::byte{'u'}, std::byte{'b'}};

  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(tx, data)};

  pqxx::bytes buf;
  PQXX_CHECK_EQUAL(pqxx::blob::append_to_buf(tx, id, 0u, buf, 1u), 1u);
  PQXX_CHECK_EQUAL(std::size(buf), 1u);
  PQXX_CHECK_EQUAL(pqxx::blob::append_to_buf(tx, id, 1u, buf, 5u), 3u);
  PQXX_CHECK_EQUAL(std::size(buf), 4u);

  PQXX_CHECK_EQUAL(buf, data);
}


void test_blob_to_file_writes_file(pqxx::test::context &tctx)
{
  pqxx::bytes const data{std::byte{'C'}, std::byte{'+'}, std::byte{'+'}};

  std::string const temp_file = tctx.make_name("blob-test");
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(tx, data)};
  pqxx::bytes buf;

  try
  {
    pqxx::blob::to_file(tx, id, temp_file);
    read_file(temp_file.c_str(), 10u, buf);
    std::ignore = std::remove(temp_file.c_str());
  }
  catch (std::exception const &)
  {
    std::ignore = std::remove(temp_file.c_str());
    throw;
  }
  PQXX_CHECK_EQUAL(buf, data);
}


void test_blob_close_leaves_blob_unusable(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto id{pqxx::blob::from_buf(tx, pqxx::bytes{std::byte{1}})};
  auto b{pqxx::blob::open_rw(tx, id)};
  b.close();
  pqxx::bytes buf;
  PQXX_CHECK_THROWS(b.read(buf, 1), pqxx::usage_error);
}


PQXX_REGISTER_TEST(test_blob_is_useless_by_default);
PQXX_REGISTER_TEST(test_blob_create_makes_empty_blob);
PQXX_REGISTER_TEST(test_blob_create_with_oid_requires_oid_be_free);
PQXX_REGISTER_TEST(test_blob_create_with_oid_obeys_oid);
PQXX_REGISTER_TEST(test_blobs_are_transactional);
PQXX_REGISTER_TEST(test_blob_remove_removes_blob);
PQXX_REGISTER_TEST(test_blob_remove_is_not_idempotent);
PQXX_REGISTER_TEST(test_blob_checks_open_mode);
PQXX_REGISTER_TEST(test_blob_supports_move);
PQXX_REGISTER_TEST(test_blob_read_reads_data);
PQXX_REGISTER_TEST(test_blob_read_reads_generic_data);
PQXX_REGISTER_TEST(test_blob_reads_vector);
PQXX_REGISTER_TEST(test_blob_read_span);
PQXX_REGISTER_TEST(test_blob_write_appends_at_insertion_point);
PQXX_REGISTER_TEST(test_blob_writes_span);
PQXX_REGISTER_TEST(test_blob_resize_shortens_to_desired_length);
PQXX_REGISTER_TEST(test_blob_resize_extends_to_desired_length);
PQXX_REGISTER_TEST(test_blob_tell_tracks_position);
PQXX_REGISTER_TEST(test_blob_seek_sets_positions);
PQXX_REGISTER_TEST(test_blob_from_buf_interoperates_with_to_buf);
PQXX_REGISTER_TEST(test_blob_append_from_buf_appends);
PQXX_REGISTER_TEST(test_blob_generic_append_from_buf_appends);
PQXX_REGISTER_TEST(test_blob_from_file_creates_blob_from_file_contents);
PQXX_REGISTER_TEST(test_blob_from_file_with_oid_writes_blob);
PQXX_REGISTER_TEST(test_blob_append_to_buf_appends);
PQXX_REGISTER_TEST(test_blob_to_file_writes_file);
PQXX_REGISTER_TEST(test_blob_close_leaves_blob_unusable);
} // namespace
