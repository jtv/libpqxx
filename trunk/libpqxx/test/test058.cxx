#include <cerrno>
#include <cstring>
#include <iostream>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";

class WriteLargeObject : public transactor<>
{
public:
  explicit WriteLargeObject() : 
    transactor<>("WriteLargeObject")
  {
  }

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T);
    cout << "Writing to large object #" << largeobject(A).id() << endl;

    A.write(Contents);

    char Buf[200];
    const size_t Size = sizeof(Buf) - 1;
    largeobjectaccess::size_type Bytes = A.read(Buf, Size);
    if (Bytes)
      throw logic_error("Could read " + to_string(Bytes) + " bytes "
	                "from large object after writing");

    // Overwrite terminating zero
    largeobjectaccess::size_type Here = A.seek(-1, ios::cur);
    if (Here != largeobjectaccess::size_type(Contents.size()-1))
      throw logic_error("Expected to move back 1 byte to " + 
	                to_string(Contents.size()-1) + ", "
			"ended up at " + to_string(Here));
    A.write("!", 1);
    
    // Now check that we really did
    A.seek(-1, ios::cur);
    if (Here != largeobjectaccess::size_type(Contents.size()-1))
      throw logic_error("Inconsistent seek: ended up at " + to_string(Here));

    char Check;
    Here = A.read(&Check, 1);
    if (Here != 1)
      throw logic_error("Wanted to read back 1 byte, got " + to_string(Here));

    if (Check != '!')
      throw logic_error("Read back '" + to_string(Check) + "', "
	                "expected '!'");

    Here = A.seek(0, ios::beg);
    if (Here != 0)
      throw logic_error("Tried to seek back to beginning of large object, "
	                "ended up at " + to_string(Here));

    Here = A.read(&Check, 1);
    if (Here != 1)
      throw logic_error("Tried to read back 1st byte, got " + 
	                to_string(Here) + " bytes");

    if (Check != Contents[0])
      throw logic_error("Expected large object to begin with '" +
	                to_string(Contents[0]) + "', "
			"found '" + to_string(Check) + "'");

    // Clean up after ourselves
    A.remove(T);
  }

private:
  largeobject m_Object;
};

}


// Mixed-mode, seeking test program for libpqxx's Large Objects interface.
//
// Usage: test058 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    C.perform(WriteLargeObject());
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
    return 1;
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


