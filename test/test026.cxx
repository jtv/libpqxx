#include <iostream>
#include <map>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Modify the database, retaining transactional
// integrity using the transactor framework, and using lazy connections.
namespace
{

// Global list of converted year numbers and what they've been converted to
map<int,int> theConversions;


// Convert year to 4-digit format.
int To4Digits(int Y)
{
  int Result = Y;

  PQXX_CHECK(Y >= 0, "Negative year: " + to_string(Y));

  if (Y  < 70)   Result += 2000;
  else if (Y  < 100)  Result += 1900;
  else PQXX_CHECK(Y >= 1970, "Unexpected year: " + to_string(Y));
  return Result;
}


// Transaction definition for year-field update
class UpdateYears : public transactor<>
{
public:
  UpdateYears() : transactor<>("YearUpdate") {}

  // Transaction definition
  void operator()(argument_type &T)
  {
    // First select all different years occurring in the table.
    result R( T.exec("SELECT year FROM pqxxevents") );

    // Note all different years currently occurring in the table, writing them
    // and their correct mappings to m_Conversions
    for (result::const_iterator r = R.begin(); r != R.end(); ++r)
    {
      int Y;

      // Read year, and if it is non-null, note its converted value
      if (r[0].to(Y))
	m_Conversions[Y] = To4Digits(Y);
    }

    // For each occurring year, write converted date back to whereever it may
    // occur in the table.  Since we're in a transaction, any changes made by
    // others at the same time will not affect us.
    for (map<int,int>::const_iterator c = m_Conversions.begin();
	 c != m_Conversions.end();
	 ++c)
      T.exec("UPDATE pqxxevents "
	     "SET year=" + to_string(c->second) + " "
	     "WHERE year=" + to_string(c->first));
  }

  // Postprocessing code for successful execution
  void on_commit()
  {
    // Report the conversions performed once the transaction has completed
    // successfully.  Do not report conversions occurring in unsuccessful
    // attempts, as some of those may have been removed from the table by
    // somebody else between our attempts.
    // Even if this fails (eg. because we run out of memory), the actual
    // database transaction will still have been performed.
    theConversions = m_Conversions;
  }

  // Postprocessing code for aborted execution attempt
  void on_abort(const char Reason[]) throw ()
  {
    try
    {
      // Notify user that the transaction attempt went wrong; we may retry.
      cerr << "Transaction interrupted: " << Reason << endl;
    }
    catch (const exception &)
    {
      // Ignore any exceptions on cerr.
    }
  }

private:
  // Local store of date conversions to be performed
  map<int,int> m_Conversions;
};


void test_026(transaction_base &)
{
  lazyconnection C;
  {
    nontransaction T(C);
    test::create_pqxxevents(T);
    T.commit();
  }

  // Perform (an instantiation of) the UpdateYears transactor we've defined
  // in the code above.  This is where the work gets done.
  C.perform(UpdateYears());

  // Just for fun, report the exact conversions performed.  Note that this
  // list will be accurate even if other people were modifying the database
  // at the same time; this property was established through use of the
  // transactor framework.
  for (map<int,int>::const_iterator i = theConversions.begin();
       i != theConversions.end();
       ++i)
    cout << '\t' << i->first << "\t-> " << i->second << endl;
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_026)
