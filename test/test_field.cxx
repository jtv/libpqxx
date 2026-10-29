#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_field(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const r1{tx.exec("SELECT 9").one_row()};
  auto const &f1{r1[0]};

  PQXX_CHECK_EQUAL(f1.as<std::string>(), "9");
  PQXX_CHECK_EQUAL(f1.as<std::string>("z"), "9");

  PQXX_CHECK_EQUAL(f1.as<int>(), 9);
  PQXX_CHECK_EQUAL(f1.as<int>(10), 9);

  std::string s;
  PQXX_CHECK(f1.to(s));
  PQXX_CHECK_EQUAL(s, "9");
  s = "x";
  PQXX_CHECK(f1.to(s, std::string{"7"}));
  PQXX_CHECK_EQUAL(s, "9");

  int i{};
  PQXX_CHECK(f1.to(i));
  PQXX_CHECK_EQUAL(i, 9);
  i = 8;
  PQXX_CHECK(f1.to(i, 12));
  PQXX_CHECK_EQUAL(i, 9);

  auto const r2{tx.exec("SELECT NULL").one_row()};
  auto const f2{r2[0]};
  i = 100;
  PQXX_CHECK_THROWS(std::ignore = f2.as<int>(), pqxx::conversion_error);
  PQXX_CHECK_EQUAL(i, 100);

  PQXX_CHECK_EQUAL(f2.as<int>(66), 66);

  PQXX_CHECK(!(f2.to(i)));
  PQXX_CHECK(!(f2.to(i, 54)));
  PQXX_CHECK_EQUAL(i, 54);

  auto const r3{tx.exec("SELECT generate_series(1, 5)")};
  PQXX_CHECK_EQUAL(r3.at(3, 0).as<int>(), 4);

  // As a special case, we can "convert a field to nullptr_t," just to make it
  // check that it is null.
  pqxx::row const rnull{tx.exec("SELECT NULL").one_row()};
  std::nullptr_t const nullv{rnull.at(0).as<std::nullptr_t>()};
  PQXX_CHECK(nullv == nullptr);
  pqxx::row const rnonnull{tx.exec("SELECT 1").one_row()};
  PQXX_CHECK_THROWS(
    std::ignore = rnonnull.as<std::nullptr_t>(), pqxx::conversion_error);

  // (Work around totally bogus MSVC warning: it thinks the comma in the C++23
  // two-parameter indexing expression is a sequence operator.  But only for
  // the purposes of issuing annoying warnings; it doesn't _actually_ believe
  // that.)
#if defined(PQXX_HAVE_MULTIDIM)
#  if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4548)
#  endif

  PQXX_CHECK_EQUAL((r3[3, 0].as<int>()), 4);

#  if defined(_MSC_VER)
#    pragma warning(pop)
#  endif

#endif // PQXX_HAVE_MULTIDIM
}


template<typename CONTAINER>
[[nodiscard]] pqxx::bytes to_vector(CONTAINER const &c)
{
  return std::vector<std::byte>{c.begin(), c.end()};
}


void test_field_converts_binary(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  auto const query{"SELECT $1::bytea"};

  pqxx::field const efield{
    tx.exec(query, pqxx::params{pqxx::bytes{}}).one_row().at(0)};
  PQXX_CHECK(not efield.is_null());

  // This is not as trivial as you might think: the server transmits the empty
  // bytea to us as a non-empty field of text data.
  PQXX_CHECK_EQUAL(efield.as<pqxx::bytes>(), pqxx::bytes{});

  // There's a specialised, optimised code path for reading binary data into
  // an existing container.
  //
  // We'll start with a nonempty container to prove that it gets overwritten.
  pqxx::bytes buf{std::byte{'n'}};
  efield.to(buf);
  PQXX_CHECK_EQUAL(buf, pqxx::bytes{});

  // Now try it with a nonempty value.  This resizes buf.
  //
  // We run it a few times, mainly to verify that resizing works in either
  // direction.
  for (int count{0}; count < 5; ++count)
  {
    auto const data{tctx.make_bytes()};
    pqxx::field const dfield{
      tx.exec(query, pqxx::params{data}).one_row().at(0)};
    PQXX_CHECK(not dfield.is_null());
    dfield.to(buf);
    PQXX_CHECK_EQUAL(std::size(buf), std::size(data));
    PQXX_CHECK_EQUAL(buf, data);
  }

  // We can even extract binary data into a non-resizable container, so long
  // as it's exactly the right size.
  // If the non-resizable container is not the right size, however, it fails.
  constexpr std::size_t len{4u};
  std::array<std::byte, len> fixed;
  for (int s{0}; std::cmp_less(s, len + 2); ++s)
  {
    auto const input{tctx.make_bytes(s, s + 1)};
    pqxx::field const afield{
      tx.exec(query, pqxx::params{input}).one_row().at(0)};
    PQXX_CHECK(not afield.is_null());
    if (std::size(input) == len)
    {
      // We're extracting the binary data in a container that can't be
      // resized, but luckily it's exactly the right size.
      afield.to(fixed);
      PQXX_CHECK_EQUAL(to_vector(fixed), input);
      PQXX_CHECK_EQUAL(to_vector(fixed), afield.as<pqxx::bytes>());
    }
    else
    {
      // Oops, no, this container is the wrong size for the data.
      PQXX_CHECK_THROWS(afield.to(fixed), pqxx::conversion_error);
    }
  }
}


PQXX_REGISTER_TEST(test_field);
PQXX_REGISTER_TEST(test_field_converts_binary);
} // namespace
