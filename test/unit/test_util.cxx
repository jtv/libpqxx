#include <cassert>

#include <pqxx/types>
#include <pqxx/util>

#include "../test_helpers.hxx"

namespace
{
template<typename T> void test_for(T const &val)
{
  auto const name{pqxx::type_name<T>};
  auto const sz{std::size(val)};

  std::span<std::byte const> out{pqxx::binary_cast(val)};

  PQXX_CHECK_EQUAL(
    std::size(out), sz,
    "Got bad size on binary_cast<" + name + "().");

  for (std::size_t i{0}; i < sz; ++i)
    PQXX_CHECK_EQUAL(
      static_cast<unsigned>(out[i]),
      static_cast<unsigned>(val[i]),
      "Mismatch in " + name + " byte " + pqxx::to_string(i) + ".");
}


void test_binary_cast()
{
  std::byte const bytes_c_array[]{
    std::byte{0x22}, std::byte{0x23}, std::byte{0x24}
  };
  test_for(bytes_c_array);
  test_for("Hello world");

  test_for(std::vector<char>{'n', 'o', 'p', 'q'});
  test_for(std::vector<unsigned char>{'n', 'o', 'p', 'q'});
  test_for(std::vector<signed char>{'n', 'o', 'p', 'q'});
}


PQXX_REGISTER_TEST(test_binary_cast);
} // namespace
