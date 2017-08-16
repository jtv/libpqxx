// Remove large objects given on the command line from the default database.
#include <iostream>

#include "pqxx/pqxx"

using namespace pqxx;

namespace
{
  class RemoveLO : public transactor<>
  {
    oid m_oid;
  public:
    explicit RemoveLO(oid O) : m_oid(O) {}

    void operator()(argument_type &T)
    {
      largeobject L(m_oid);
      L.remove(T);
    }
  };
}

int main(int, char *argv[])
{
  lazyconnection C;
  bool failures = false;

  try
  {
    for (int i=1; argv[i]; ++i)
    {
      oid O;
      from_string(argv[i], O);
      try
      {
        C.perform(RemoveLO(O));
      }
      catch (const std::exception &e)
      {
        std::cerr << e.what() << std::endl;
        failures = true;
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return 2;
  }

  return failures;
}
