#include <iostream>
#include <sstream>

#include "../test_helpers.hxx"


namespace
{
void test_stream_large_object()
{
  pqxx::connection conn;

  // Construct a really nasty string.  (Don't just construct a std::string from
  // a char[] constant, because it'll terminate at the embedded zero.)
  //
  // The crucial thing is the "ff" byte at the beginning.  It tests for
  // possible conflation between "eof" (-1) and a char which just happens to
  // have the same bit pattern as an 8-bit value of -1.  This conflation can be
  // a problem when it occurs at buffer boundaries.
  char const bytes[]{"\xff\0end"};
  std::string const contents{bytes, sizeof(bytes)};

  pqxx::work tx{conn};
  pqxx::largeobject new_obj{tx};

  pqxx::olostream write{tx, new_obj};
  write << contents;
  write.flush();

  pqxx::largeobjectaccess check{tx, new_obj, std::ios::in | std::ios::binary};
  std::array<char, 50> buf;
  std::size_t const len{
    static_cast<std::size_t>(check.read(buf.data(), buf.size()))};
  PQXX_CHECK_EQUAL(len, contents.size(), "olostream truncated data.");
  std::string const check_str{buf.data(), len};
  PQXX_CHECK_EQUAL(check_str, contents, "olostream mangled data.");

  pqxx::ilostream read{tx, new_obj};
  std::string read_back;
  std::string chunk;
  while (read >> chunk) read_back += chunk;

  new_obj.remove(tx);

  PQXX_CHECK_EQUAL(read_back, contents, "Got wrong data from ilostream.");
  PQXX_CHECK_EQUAL(
    read_back.size(), contents.size(), "ilostream truncated data.");
  PQXX_CHECK_EQUAL(
    read_back.size(), sizeof(bytes), "ilostream truncated data.");
}


PQXX_REGISTER_TEST(test_stream_large_object);
} // namespace
