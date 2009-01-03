// Print thread-safety information for present libpqxx build.
#include <iostream>

#include "pqxx/util"

using namespace PGSTD;

int main()
{
  cout << pqxx::describe_thread_safety().description << endl;
}
