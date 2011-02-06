#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


namespace
{
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


void test_025(transaction_base &T)
{
  // Set up two connections to the backend: one to read our original table,
  // and another to write our copy
  lazyconnection orgC(""), dstC(NULL);

  // Select our original and destination table names
  const string orgTable = "pqxxorgevents";
  const string dstTable = "pqxxevents";

  T.exec("DROP TABLE IF EXISTS " + dstTable);
  T.exec("CREATE TABLE " + dstTable + "(year INTEGER, event TEXT)");

  // Set up a transaction to access the original table from
  work orgTrans(orgC, "test25org");
  dstC.perform(CopyTable(orgTrans, orgTable, dstTable));
}
} // namespace

PQXX_REGISTER_TEST_T(test_025, nontransaction)
