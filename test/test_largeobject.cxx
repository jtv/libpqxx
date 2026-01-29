#include <sstream>

#include <pqxx/largeobject>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_stream_large_object(pqxx::test::context &)
{
  pqxx::connection cx;

  // Construct a really nasty string.  (Don't just construct a std::string from
  // a char[] constant, because it'll terminate at the embedded zero.)
  //
  // The crucial thing is the "ff" byte at the beginning.  It tests for
  // possible conflation between "eof" (-1) and a char which just happens to
  // have the same bit pattern as an 8-bit value of -1.  This conflation can be
  // a problem when it occurs at buffer boundaries.
  constexpr char bytes[]{"\xff\0end"};
  std::string const contents{std::data(bytes), std::size(bytes)};

  pqxx::work tx{cx};
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::largeobject const new_obj{tx};

  pqxx::olostream write{tx, new_obj};
  write << contents;
  write.flush();

  pqxx::largeobjectaccess check{tx, new_obj, std::ios::in | std::ios::binary};
  std::array<char, 50> buf{};
  std::size_t const len{
    static_cast<std::size_t>(check.read(std::data(buf), std::size(buf)))};
  PQXX_CHECK_EQUAL(len, std::size(contents));
  std::string const check_str{std::data(buf), len};
  PQXX_CHECK_EQUAL(check_str, contents);

  pqxx::ilostream read{tx, new_obj};
  std::string read_back;
  std::string chunk;
  while (read >> chunk) read_back += chunk;

  new_obj.remove(tx);

  PQXX_CHECK_EQUAL(read_back, contents);
  PQXX_CHECK_EQUAL(std::size(read_back), std::size(contents));
  PQXX_CHECK_EQUAL(std::size(read_back), std::size(bytes));
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


PQXX_REGISTER_TEST(test_stream_large_object);
} // namespace
