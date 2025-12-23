#include <iostream>

#if !__has_cpp_attribute(gnu::null_terminated_string_arg)
#  error "This compiler does not support gnu::null_terminated_string_arg."
#endif

[[gnu::null_terminated_string_arg]] void print(char const *msg)
{
  std::cout << msg << '\n';
}


int main()
{
  print("zargs works.");
}
