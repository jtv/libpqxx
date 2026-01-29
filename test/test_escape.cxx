#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
using namespace std::literals;
using pqxx::operator""_zv;

void compare_esc(
  pqxx::connection &cx, pqxx::transaction_base &t, char const text[])
{
  std::size_t const len{std::size(std::string{text})};
  PQXX_CHECK_EQUAL(
    cx.esc(std::string_view{text, len}), t.esc(std::string_view{text, len}));

  PQXX_CHECK_EQUAL(t.esc(std::string_view{text, len}), t.esc(text));

  PQXX_CHECK_EQUAL(t.esc(std::string{text}), t.esc(text));

  PQXX_CHECK_EQUAL(
    text, t.query_value<std::string>(
            "SELECT '" + t.esc(std::string_view{text, len}) + "'"));

  PQXX_CHECK_EQUAL(t.esc(std::string_view{text, len}), t.esc(text));
}


void test_esc(pqxx::connection &cx, pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.esc(""sv), "");
  PQXX_CHECK_EQUAL(t.esc("'"sv), "''");
  PQXX_CHECK_EQUAL(t.esc("hello"sv), "hello");
  std::array<char const *, 4> const escstrings{"x", " ", "", nullptr};
  for (std::size_t i{0}; escstrings.at(i) != nullptr; ++i)
    compare_esc(cx, t, escstrings.at(i));
}


void test_quote(pqxx::connection &cx, pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.quote("x"), "'x'");
  PQXX_CHECK_EQUAL(t.quote(1), "'1'");
  PQXX_CHECK_EQUAL(t.quote(0), "'0'");
  char const *const null_ptr{nullptr};
  PQXX_CHECK_EQUAL(t.quote(null_ptr), "NULL");
  PQXX_CHECK_EQUAL(t.quote(nullptr), "NULL");
  PQXX_CHECK_EQUAL(t.quote(std::string{"'"}), "''''");

  PQXX_CHECK_EQUAL(t.quote("x"), cx.quote("x"));

  std::array<char const *, 10> const test_strings{
    "", "x", "\\", "\\\\", "'", "''", "\\'", "\t", "\n", nullptr};

  for (std::size_t i{0}; test_strings.at(i) != nullptr; ++i)
  {
    auto r{
      t.query_value<std::string>("SELECT " + t.quote(test_strings.at(i)))};
    PQXX_CHECK_EQUAL(r, test_strings.at(i));
  }

  std::vector<std::byte> const bin{std::byte{0x33}, std::byte{0x4a}};
  PQXX_CHECK_EQUAL(t.quote(bin), "'\\x334a'::bytea");
  PQXX_CHECK_EQUAL(
    t.quote(std::span<std::byte const>{bin}), "'\\x334a'::bytea");
}


void test_quote_name(pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL("\"A b\"", t.quote_name("A b"));
  PQXX_CHECK_EQUAL(
    std::string{"A b"},
    t.exec("SELECT 1 AS " + t.quote_name("A b")).column_name(0));
}


void test_esc_raw_unesc_raw(pqxx::transaction_base &t)
{
  constexpr char binary[]{"1\002.3\\4x5"};

  // C++23: Initialise as data{std::from_range_t, binary)?
  pqxx::bytes data;
  for (char c : binary) data.push_back(static_cast<std::byte>(c));

  std::string const escaped{
    t.esc(pqxx::bytes_view{std::data(data), std::size(binary)})};

  for (auto const i : escaped)
  {
    PQXX_CHECK_GREATER(
      static_cast<unsigned>(static_cast<unsigned char>(i)), 7u,
      "Non-ASCII character in escaped data: " + escaped);
    PQXX_CHECK_LESS(
      static_cast<unsigned>(static_cast<unsigned char>(i)), 127u,
      "Non-ASCII character in escaped data: " + escaped);
  }

  for (auto const i : escaped)
    PQXX_CHECK(
      isprint(i), "Unprintable character in escaped data: " + escaped);

  PQXX_CHECK_EQUAL(escaped, "\\x31022e335c34783500");
  PQXX_CHECK_EQUAL(std::size(t.unesc_bin(escaped)), std::size(data));
  auto unescaped{t.unesc_bin(escaped)};
  PQXX_CHECK_EQUAL(std::size(unescaped), std::size(data));
  for (std::size_t i{0}; i < std::size(unescaped); ++i)
    PQXX_CHECK_EQUAL(
      int(unescaped[i]), int(data[i]),
      "Unescaping binary data did not restore byte #" + pqxx::to_string(i) +
        ".");

  PQXX_CHECK_THROWS(std::ignore = t.unesc_bin("\\"_zv), pqxx::failure);
  PQXX_CHECK_THROWS(std::ignore = t.unesc_bin("\\xa"_zv), pqxx::failure);
  PQXX_CHECK_THROWS(std::ignore = t.unesc_bin("\\xg0"_zv), pqxx::failure);
  PQXX_CHECK_THROWS(std::ignore = t.unesc_bin("\\x0g"_zv), pqxx::failure);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::internal::unesc_bin("\\a"sv, pqxx::sl::current()),
    pqxx::failure);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::internal::unesc_bin("\\"sv, pqxx::sl::current()),
    pqxx::failure);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::internal::unesc_bin("\\\xa"sv, pqxx::sl::current()),
    pqxx::failure);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::internal::unesc_bin("\\\a"sv, pqxx::sl::current()),
    pqxx::failure);
}


void test_esc_like(pqxx::transaction_base &tx)
{
  PQXX_CHECK_EQUAL(tx.esc_like(""), "");
  PQXX_CHECK_EQUAL(tx.esc_like("abc"), "abc");
  PQXX_CHECK_EQUAL(tx.esc_like("_"), "\\_");
  PQXX_CHECK_EQUAL(tx.esc_like("%"), "\\%");
  PQXX_CHECK_EQUAL(tx.esc_like("a%b_c"), "a\\%b\\_c");
  PQXX_CHECK_EQUAL(tx.esc_like("_", '+'), "+_");
  PQXX_CHECK_EQUAL(tx.esc_like("%foo"), "\\%foo");
  PQXX_CHECK_EQUAL(tx.esc_like("foo%"), "foo\\%");
  PQXX_CHECK_EQUAL(tx.esc_like("f%o%o"), "f\\%o\\%o");
  PQXX_CHECK_EQUAL(tx.esc_like("___"), "\\_\\_\\_");
  PQXX_CHECK_EQUAL(tx.esc_like("_n_n__n_"), "\\_n\\_n\\_\\_n\\_");
}


void test_escaping(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  test_esc(cx, tx);
  test_quote(cx, tx);
  test_quote_name(tx);
  test_esc_raw_unesc_raw(tx);
  test_esc_like(tx);
}


void test_esc_escapes_into_buffer(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work const tx{cx};

  std::string buffer;
  buffer.resize(20);

  auto const text{"Ain't"sv};
  auto escaped_text{tx.esc(text, buffer)};
  PQXX_CHECK_EQUAL(escaped_text, "Ain''t");

  pqxx::bytes const data{std::byte{0x22}, std::byte{0x43}};
  auto escaped_data(tx.esc(data, buffer));
  PQXX_CHECK_EQUAL(escaped_data, "\\x2243");
}


void test_esc_accepts_various_types(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work const tx{cx};

  std::string buffer;
  buffer.resize(20);

  std::string const text{"it's"};
  auto escaped_text{tx.esc(text, buffer)};
  PQXX_CHECK_EQUAL(escaped_text, "it''s");

  std::vector<std::byte> const data{std::byte{0x23}, std::byte{0x44}};
  auto escaped_data(tx.esc(data, buffer));
  PQXX_CHECK_EQUAL(escaped_data, "\\x2344");
}


void test_binary_esc_checks_buffer_length(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::string buf;
  pqxx::bytes bin{std::byte{'b'}, std::byte{'o'}, std::byte{'o'}};

  buf.resize(2 * std::size(bin) + 3);
  std::ignore = tx.esc(bin, buf);
  PQXX_CHECK_EQUAL(int{buf[0]}, int{'\\'}, "Unexpected binary escape format.");
  PQXX_CHECK_NOT_EQUAL(
    int(buf[std::size(buf) - 2]), int('\0'), "Escaped binary ends too soon.");
  PQXX_CHECK_EQUAL(
    int(buf[std::size(buf) - 1]), int('\0'), "Terminating zero is missing.");

  buf.resize(2 * std::size(bin) + 2);
  PQXX_CHECK_THROWS(std::ignore = tx.esc(bin, buf), pqxx::range_error);
}


PQXX_REGISTER_TEST(test_escaping);
PQXX_REGISTER_TEST(test_esc_escapes_into_buffer);
PQXX_REGISTER_TEST(test_esc_accepts_various_types);
PQXX_REGISTER_TEST(test_binary_esc_checks_buffer_length);
} // namespace
