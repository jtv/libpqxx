// Test for std::span.
// C++20: Assume support.
#include <span>

int main(int argc, char **argv)
{
  std::span<char *> args{argv, static_cast<std::size_t>(argc)};
  return static_cast<int>(std::size(args) - 1u);
}
