#include <stacktrace>

int main()
{
  return std::stacktrace::current().size() > 0u;
}
