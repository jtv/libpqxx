#include <pqxx/hstore>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
inline pqxx::conversion_context make_context(
  pqxx::connection const &cx,
  std::source_location loc = std::source_location::current())
{
  return pqxx::conversion_context{cx.get_encoding_group(), loc};
}


template<typename KEY, typename VALUE>
std::vector<std::pair<KEY, VALUE>>
parse_hstore(std::string_view input, pqxx::ctx c)
{
  std::vector<std::pair<KEY, VALUE>> data;
  for (auto const &[key, value] : pqxx::hstore_parse<KEY, VALUE>(input, c))
    data.emplace_back(key, value);
  return data;
}

void test_hstore(pqxx::test::context &ctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  if (not pqxx::test::have_extension(tx, "hstore"))
    return;

  auto const key{ctx.make_name("k")}, value{ctx.make_name("v")};
  PQXX_CHECK_EQUAL(
    tx.query_value<std::string>(
      "SELECT ($1::hstore)[$2]",
      pqxx::params{std::format("{}=>{}", key, value), key}),
    value);

  auto const empty_data{
    parse_hstore<std::string, std::string>("", make_context(cx))};
  PQXX_CHECK(empty_data.empty());

  // XXX: Test empty hstore.
  // XXX: Test tolerance for trailing comma.
  // XXX: Test more!
}
} // namespace


PQXX_REGISTER_TEST(test_hstore);
