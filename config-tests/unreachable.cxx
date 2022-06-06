// Test for C++23 std::unreachable.
#include <utility>

int main(int argc, char **)
{
  if (argc < 100)
  {
    return 0;
  }
  else
  {
    std::unreachable();
  }
  return 1;
}
