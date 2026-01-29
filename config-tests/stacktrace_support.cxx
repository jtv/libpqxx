#include <format>
#include <iostream>
#include <stacktrace>

int main()
{
  std::cout << std::format("{}", std::stacktrace::current());
}
