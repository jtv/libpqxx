#include <iostream>
#include <stdexcept>
#include <vector>

#include "pg_connection.h"
#include "pg_tablereader.h"
#include "pg_tablewriter.h"
#include "pg_transaction.h"

using namespace PGSTD;
using namespace Pg;


// Test program for libpqxx.  Copy a table from one database connection to 
// another using a TableReader and a TableWriter.  Any data already in the
// destination table is overwritten.
//
// Usage: test6 [connect-string] [orgtable] [dsttable]
//
// Where the connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend 
// running on host foo.bar.net, logging in as user smith.
//
// The sample program assumes that both orgtable and dsttable are tables that
// exist in the database that connect-string (whether the default or one
// specified explicitly on the command line) connects to.
//
// The default origin table name is "events" as used by other test programs.
// PostgreSQL currently implements pg_tables as a view, which cannot be read by
// using the COPY command.  Otherwise, pg_tables would have made a better 
// default value here.  The default destination table is the origin table name
// with "copy" appended.

namespace
{

class ClearTable : public Transactor
{
  string m_Table;

public:
  ClearTable(string Table) : m_Table(Table) {}

  void operator()(Transaction &T)
  {
    T.Exec(("DELETE FROM " + m_Table).c_str());
  }

  void OnCommit()
  {
    cout << "Table successfully cleared." << endl;
  }
};


void CheckState(TableReader &R)
{
  if (!R != !bool(R))
    throw logic_error("TableReader " + R.Name() + " in inconsistent state!");
}


class CopyTable : public Transactor
{
  Transaction &m_orgTrans;  // Transaction giving us access to original table
  string m_orgTable;	    // Original table's name
  string m_dstTable;	    // Destination table's name

public:
  // Constructor--pass parameters for operation here
  CopyTable(Transaction &OrgTrans, string OrgTable, string DstTable) :
    Transactor("CopyTable"),
    m_orgTrans(OrgTrans),
    m_orgTable(OrgTable),
    m_dstTable(DstTable)
  {
  }

  // Transaction definition
  void operator()(Transaction &T)
  {
    TableReader Org(m_orgTrans, m_orgTable);
    TableWriter Dst(T, m_dstTable);

    CheckState(Org);

    // Copy table Org into table Dst.  This transfers all the data to the 
    // frontend and back to the backend.  Since in this example Ord and Dst are
    // really in the same database, we'd do this differently in real life; a
    // simple SQL query would suffice.
    Dst << Org;

    CheckState(Org);
  }

  void OnCommit()
  {
    cout << "Table successfully copied." << endl;
  }
};


}

int main(int argc, char *argv[])
{
  try
  {
    const char *ConnStr = (argv[1] ? argv[1] : "");

    // Set up two connections to the backend: one to read our original table,
    // and another to write our copy
    Connection orgC(ConnStr), dstC(ConnStr);

    // Select our original and destination table names
    string orgTable = "events";
    if (argc > 2) orgTable = argv[2];

    string dstTable = orgTable + "copy";
    if (argc > 3) dstTable = argv[3];

    // Set up a transaction to access the original table from
    Transaction orgTrans(orgC, "test6org");

    dstC.Perform(ClearTable(dstTable));
    dstC.Perform(CopyTable(orgTrans, orgTable, dstTable));
  }
  catch (const exception &e)
  {
    // All exceptions thrown by libpqxx are derived from std::exception
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    // This is really unexpected (see above)
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}

