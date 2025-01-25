// Check whether we need to link to the stdc++fs library.
//
// This check will succeed if std::filesystem::path works without any special
// options.  If the link fails, we assume that -lstdc++fs will fix it for us.

#include <filesystem>
#include <iostream>


int main()
{
  std::cout << std::filesystem::path{"foo.bar"}.c_str() << '\n';
}
