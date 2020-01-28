// Test for std::to_string/std::from_string for floating-point types.
#include <iostream>
#include <sstream>

int main(int argc, char **)
{
  thread_local std::stringstream s;
  s << argc;
  std::cout << s.str();
}
