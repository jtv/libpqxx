// Remove large objects given on the command line from the default database
#include <iostream>

#include "pqxx/pqxx"

using namespace std;
using namespace pqxx;

namespace
{
  class RemoveLO : public transactor<>
  {
    Oid m_O;
  public:
    explicit RemoveLO(Oid O) : m_O(O) {}

    void operator()(argument_type &T)
    {
      largeobject L(m_O);
      L.remove(T);
    }
  };
}

int main(int, char *argv[])
{
  lazyconnection C;
  bool Failures = false;

  try
  {
    for (int i=1; argv[i]; ++i)
    {
      Oid O;
      FromString(argv[i], O);
      try
      {
        C.perform(RemoveLO(O));
      }
      catch (const exception &e)
      {
        cerr << e.what() << endl;
        Failures = true;
      }
    }
  }
  catch (const exception &e)
  {
    cerr << e.what() << endl;
    return 2;
  }

  return Failures;
}

