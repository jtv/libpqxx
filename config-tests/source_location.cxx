#include <source_location>

int main()
{
  return static_cast<int>(std::source_location::current().line());
}
