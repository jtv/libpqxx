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


template<typename KEY, typename VALUE>
void check_empty_parse(pqxx::connection const &cx)
{
  auto const c{make_context(cx)};
  pqxx::hstore_parse<KEY, VALUE> const blank{"", c};
  PQXX_CHECK(blank.begin() == blank.end());
  PQXX_CHECK(blank.cbegin() == blank.cend());

  pqxx::hstore_parse<KEY, VALUE> const spaces{" \n \t ", c};
  PQXX_CHECK(spaces.begin() == spaces.end());
  PQXX_CHECK(spaces.cbegin() == spaces.cend());
}


void test_hstore_parse_parses_hstore(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  if (not pqxx::test::have_extension(tx, "hstore"))
    return;

  check_empty_parse<std::string, std::string>(cx);
  check_empty_parse<std::string_view, int>(cx);
  check_empty_parse<int, std::string_view>(cx);
  check_empty_parse<bool, double>(cx);

  auto const c{make_context(cx)};

  // XXX: Expecting "=>" at the end!?
  auto const basic_ints{parse_hstore<int, int>("1=>2", c)};
  PQXX_CHECK_EQUAL(std::size(basic_ints), 1u);
  PQXX_CHECK_EQUAL(basic_ints.at(0).first, 1);
  PQXX_CHECK_EQUAL(basic_ints.at(0).second, 2);
}


void test_simple_hstore(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  if (not pqxx::test::have_extension(tx, "hstore"))
    return;

  auto const empty_data{
    parse_hstore<std::string, std::string>("", make_context(cx))};
  PQXX_CHECK(empty_data.empty());
}

// XXX: Test parsing of single-entry hstores.
// XXX: Test escaping of multibyte chars.
} // namespace


PQXX_REGISTER_TEST(test_simple_hstore);
PQXX_REGISTER_TEST(test_hstore_parse_parses_hstore);
