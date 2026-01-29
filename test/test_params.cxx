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

  pqxx::params p, q;
  p.append();
  p.append("zview"_zv);
  q.append(bin);
  p.append(q);
  auto const res{tx.exec("SELECT $1, $2, $3", p)};
  PQXX_CHECK(res.at(0).at(0).is_null());
  PQXX_CHECK_EQUAL(res.at(0).at(1).view(), "zview");
  PQXX_CHECK_EQUAL(res.at(0).at(2).view(), "ab");
}


PQXX_REGISTER_TEST(test_statement_params);
} // namespace
