// Test for concepts support.  Not just the language feature; also headers.
#include <iostream>
#include <ranges>
#include <vector>


template<typename T> concept Foo = std::ranges::input_range<T>;


template<Foo F> auto foo(F const &r)
{
  return std::distance(r.begin(), r.end());
}

int main()
{
  std::vector<int> const v{1, 2, 3};
  std::cout << foo(v) << '\n';
}
