// Print thread-safety information for present libpqxx build.
#include <iostream>

#include "pqxx/util"


// TODO: No longer useful as of PostgreSQL 17: libpq is always thread-safe.
int main()
{
  std::cout << pqxx::describe_thread_safety().description << std::endl;
}
