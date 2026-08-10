#include <pqxx/pqxx>

#include "helpers.hxx"


namespace
{
using pqxx::operator""_zv;


void test_statement_params(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::array<std::byte, 2> const bin{std::byte{'a'}, std::byte{'b'}};

  pqxx::params p;
  p.append();
  p.append("zview"_zv);

  {
    pqxx::params q;
    q.append(bin);
    p.append(q);
  }

  p.append(pqxx::bytes{});
  p.append(std::optional<pqxx::bytes>{});
  p.append(std::optional{bin});
  {
    const auto c_params = p.make_c_params({});
    constexpr auto binary = static_cast<int>(pqxx::format::binary);
    constexpr auto text = static_cast<int>(pqxx::format::text);
    PQXX_CHECK_EQUAL(c_params.formats[2], binary);
    PQXX_CHECK(c_params.values[4] == nullptr);
    PQXX_CHECK_EQUAL(c_params.formats[5], binary);
  }

  auto const res{tx.exec("SELECT $1, $2, $3, $4, $5, $6", p)};
  const auto &row = res.at(0);
  PQXX_CHECK(row.at(0).is_null());
  PQXX_CHECK_EQUAL(row.at(1).view(), "zview");
  PQXX_CHECK_EQUAL(row.at(2).view(), "ab");
  PQXX_CHECK(not row.at(3).is_null());
  PQXX_CHECK_EQUAL(row.at(3).view(), "");
  PQXX_CHECK(row.at(4).is_null());
  PQXX_CHECK(not row.at(5).is_null());
  PQXX_CHECK_EQUAL(row.at(5).view(), "ab");
}


PQXX_REGISTER_TEST(test_statement_params);
} // namespace
