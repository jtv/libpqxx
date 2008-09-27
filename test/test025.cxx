#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Copy a table from one database connection to
// another using a tablereader and a tablewriter.  Any data already in the
// destination table is overwritten.  Lazy connections are used.
namespace
{
class CreateTable : public transactor<>
{
  string m_Table;

public:
  CreateTable(string Table) : transactor<>("CreateTable"), m_Table(Table) {}

  void operator()(argument_type &T)
  {
    T.exec("CREATE TABLE " + m_Table + "(year INTEGER, event TEXT)");
    cout << "Table " << m_Table << " created." << endl;
  }
};

class ClearTable : public transactor<>
{
  string m_Table;

public:
  ClearTable(string Table) : transactor<>("ClearTable"), m_Table(Table) {}

  void operator()(argument_type &T)
  {
    T.exec("DELETE FROM " + m_Table);
  }

  void on_commit()
  {
    cout << "Table successfully cleared." << endl;
  }
};


void CheckState(tablereader &R)
{
  PQXX_CHECK_EQUAL(
	!R,
	!bool(R),
	"tablereader " + R.name() + " is in an inconsistent state.");
}


class CopyTable : public transactor<>
{
  dbtransaction &m_orgTrans; // Transaction giving us access to original table
  string m_orgTable;	     // Original table's name
  string m_dstTable;	     // Destination table's name

public:
  // Constructor--pass parameters for operation here
  CopyTable(dbtransaction &OrgTrans, string OrgTable, string DstTable) :
    transactor<>("CopyTable"),
    m_orgTrans(OrgTrans),
    m_orgTable(OrgTable),
    m_dstTable(DstTable)
  {
  }

  // Transaction definition
  void operator()(argument_type &T)
  {
    tablereader Org(m_orgTrans, m_orgTable);
    tablewriter Dst(T, m_dstTable);

    CheckState(Org);

    // Copy table Org into table Dst.  This transfers all the data to the
    // frontend and back to the backend.  Since in this example Ord and Dst are
    // really in the same database, we'd do this differently in real life; a
    // simple SQL query would suffice.
    Dst << Org;

    CheckState(Org);
  }

  void on_commit()
  {
    cout << "Table successfully copied." << endl;
  }
};


void test_025(connection_base &, transaction_base &)
{
  // Set up two connections to the backend: one to read our original table,
  // and another to write our copy
  lazyconnection orgC(""), dstC(NULL);

  // Select our original and destination table names
  const string orgTable = "pqxxorgevents";
  const string dstTable = "pqxxevents";

  // Set up a transaction to access the original table from
  work orgTrans(orgC, "test25org");

  // Attempt to create table.  Ignore errors, as they're probably one of:
  // (1) Table already exists--fine with us
  // (2) Something else is wrong--we'll just fail later on anyway
  try
  {
    dstC.perform(CreateTable(dstTable));
  }
  catch (const sql_error &)
  {
  }

  dstC.perform(ClearTable(dstTable));
  dstC.perform(CopyTable(orgTrans, orgTable, dstTable));
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_025)
