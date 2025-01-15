#if !__has_cpp_attribute(assume)
#  error "No support for [[assume]] attribute."
#endif

#include <iostream>


void foo(int i)
{
  [[assume(i > 0)]];
  std::cout << i << '\n';
}
