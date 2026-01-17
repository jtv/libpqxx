#include <meta>

template<typename T> constexpr std::string_view name_type()
{
  return display_string_of(^^T);
}


int main()
{
  static_assert(name_type<int>() == "int");
}
