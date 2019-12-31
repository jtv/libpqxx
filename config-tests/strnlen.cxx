// Test for strnlen.  (May or may not be in std.)
#include <cstring>

#if __has_include(<string.h>)
#  include <string.h>
#endif


int main()
{
  using namespace std;
  return static_cast<int>(strnlen("Foo", 0));
}
