// Remove large objects given on the command line from the default database
#include <iostream>

#include "pqxx/all.h"

using namespace std;
using namespace pqxx;

namespace
{
  class RemoveLO : public Transactor
  {
    Oid m_O;
  public:
    explicit RemoveLO(Oid O) : m_O(O) {}

    void operator()(argument_type &T)
    {
      LargeObject L(m_O);
      L.remove(T);
    }
  };
}

int main(int argc, char *argv[])
{
  Connection C;
  bool Failures = false;

  try
  {
    for (int i=1; argv[i]; ++i)
    {
      Oid O;
      FromString(argv[i], O);
      try
      {
        C.Perform(RemoveLO(O));
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

