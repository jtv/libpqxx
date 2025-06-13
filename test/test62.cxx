#include <cstring>
#include <iostream>

#include <pqxx/transaction>

#include "helpers.hxx"


// Example program for libpqxx.  Test binary string functionality.
namespace
{
void test_062()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::string const TestStr{
    "Nasty\n\030Test\n\t String with \200\277 weird bytes "
    "\r\0 and Trailer\\\\\0"};

  tx.exec("CREATE TEMP TABLE pqxxbin (binfield bytea)").no_rows();

  // C++23: Initialise as data{std::from_range_t, TestStr}?
  pqxx::bytes data;
  for (char c : TestStr) data.push_back(static_cast<std::byte>(c));

  std::string const Esc{tx.esc_raw(data)};

  tx.exec("INSERT INTO pqxxbin VALUES ('" + Esc + "')").no_rows();

  pqxx::result const R{tx.exec("SELECT * from pqxxbin")};
  tx.exec("DELETE FROM pqxxbin").no_rows();

  auto const B{R.at(0).at(0).as<pqxx::bytes>()};

  PQXX_CHECK(not std::empty(B), "Binary string became empty in conversion.");

  PQXX_CHECK_EQUAL(
    std::size(B), std::size(TestStr), "Binary string was mangled.");

  pqxx::bytes::const_iterator c;
  pqxx::bytes::size_type i{};
  for (i = 0, c = std::begin(B); i < std::size(B); ++i, ++c)
  {
    PQXX_CHECK(c != std::end(B), "Premature end to binary string.");

    char const x{TestStr.at(i)}, y{char(B.at(i))}, z{char(std::data(B)[i])};

    PQXX_CHECK_EQUAL(
      std::string(&x, 1), std::string(&y, 1), "Binary string byte changed.");

    PQXX_CHECK_EQUAL(
      std::string(&y, 1), std::string(&z, 1),
      "Inconsistent byte at offset " + pqxx::to_string(i) + ".");
  }
}


PQXX_REGISTER_TEST(test_062);
} // namespace
