// Remove large objects given on the command line from the default database.
#include <iostream>

#include "pqxx/pqxx"


int main(int, char *argv[])
{
  pqxx::connection C;
  bool failures = false;

  try
  {
    for (int i{1}; argv[i]; ++i)
    {
      auto O{pqxx::from_string<pqxx::oid>(argv[i])};
      try
      {
        pqxx::perform([O, &C] {
          pqxx::work w{C};
          pqxx::largeobject l{O};
          l.remove(w);
          w.commit();
        });
      }
      catch (std::exception const &e)
      {
        std::cerr << e.what() << std::endl;
        failures = true;
      }
    }
  }
  catch (std::exception const &e)
  {
    std::cerr << e.what() << std::endl;
    return 2;
  }

  return failures;
}
