// Test for sufficient std::variant support.
#include <iostream>
#include <string>
#include <variant>

int main()
{
  std::variant<int, std::string> v{"Hello"};
  std::visit([](auto const &i) { std::cout << i; }, v);
}
