// Test for std::to_string/std::from_string for floating-point types.
#include <iostream>
#include <sstream>

int main(int argc, char **)
{
#if defined(__MINGW32__) && defined(__GNUC__) && __GNUC__ < 11
#  error "This MinGW gcc's thread_local breaks at run time, sorry."
#endif
  thread_local std::stringstream s;
  s << argc;
  std::cout << s.str();
}
