// Remove large objects given on the command line from the default database.
#include <iostream>

#include "pqxx/pqxx"

using namespace pqxx;


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
        pqxx::perform([O, &C]{
          work w{C};
          largeobject l{O};
          l.remove(w);
          w.commit();
          });
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
