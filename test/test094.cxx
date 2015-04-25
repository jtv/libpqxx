#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Simulate "in-doubt" transaction failure.
namespace
{
// Transaction class that simulates connection failure during commit.
class basic_flakytransaction PQXX_FINAL : public dbtransaction
{
public:
  bool simulate_failure;

protected:
  basic_flakytransaction(connection_base &C, const string &I) :
    namedclass("flakytransaction"),
    dbtransaction(C, I),
    simulate_failure(false)
	{}

private:
  virtual void do_commit() PQXX_OVERRIDE
  {
    if (simulate_failure) conn().simulate_failure();

    try
    {
      DirectExec("COMMIT");
    }
    catch (const exception &e)
    {
      if (conn().is_open())
      {
        PQXX_CHECK(!simulate_failure, "Connection did not simulate failure.");
	cerr << "Unexpected exception (connection still open)" << endl;
	throw;
      }

      process_notice(e.what() + string("\n"));

      const string Msg =
	(simulate_failure ?
		"Simulating commit failure" :
		"UNEXPECTED COMMIT FAILURE");

      process_notice(Msg + "\n");
      throw in_doubt_error(Msg);
    }
  }
};


template<isolation_level ISOLATIONLEVEL=read_committed>
class flakytransaction : public basic_flakytransaction
{
public:
  typedef isolation_traits<ISOLATIONLEVEL> isolation_tag;

  explicit flakytransaction(connection_base &C, const string &TName) :
    namedclass(fullname("transaction",isolation_tag::name()), TName),
    basic_flakytransaction(C, isolation_tag::name())
	{ Begin(); }

  explicit flakytransaction(connection_base &C) :
    namedclass(fullname("transaction",isolation_tag::name())),
    basic_flakytransaction(C, isolation_tag::name())
	{ Begin(); }

  virtual ~flakytransaction() PQXX_NOEXCEPT { End(); }
};


// A transactor build to fail, at least for the first m_failcount commits
class FlakyTransactor : public transactor<flakytransaction<> >
{
  int m_failcount;
public:
  explicit FlakyTransactor(int failcount=0) : 
    transactor<flakytransaction<> >("FlakyTransactor"),
    m_failcount(failcount)
	{}

  void operator()(argument_type &T)
  {
    T.simulate_failure = (m_failcount > 0);

    T.exec("SELECT count(*) FROM pg_tables");
  }

  void on_doubt() PQXX_NOEXCEPT
  {
    try
    {
      if (m_failcount > 0)
      {
        --m_failcount;
        pqxx::test::expected_exception("Transactor outcome in doubt.");
      }
      else
      {
        cerr << "Transactor outcome in doubt!" << endl;
      }
    }
    catch (const exception &)
    {
    }
  }
};


void test_094(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  // Run without simulating failure
  C.perform(FlakyTransactor(0), 1);

  // Simulate one failure.  The transactor will succeed on a second attempt, but
  // since this is an in-doubt error, the framework does not retry.
  PQXX_CHECK_THROWS(
	C.perform(FlakyTransactor(1), 2),
	in_doubt_error,
	"Simulated failure did not lead to in-doubt error.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_094, nontransaction)
