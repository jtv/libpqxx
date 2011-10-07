#include <vector>

#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
class TestErrorHandler : public errorhandler
{
public:
  TestErrorHandler(
	connection_base &c,
	vector<TestErrorHandler *> &activated_handlers,
	bool retval=true) :
    errorhandler(c),
    return_value(retval),
    message(),
    handler_list(activated_handlers)
  {}

  virtual bool operator()(const char msg[]) throw ()
  {
    message = string(msg);
    handler_list.push_back(this);
    return return_value;
  }

  bool return_value;
  string message;
  vector<TestErrorHandler *> &handler_list;
};
}

namespace pqxx
{
template<> struct string_traits<TestErrorHandler *>
{
  typedef TestErrorHandler *subject_type;
  static const char *name() { return "TestErrorHandler"; }
  static bool has_null() { return true; }
  static bool is_null(TestErrorHandler *t) { return !t; }
  static string to_string(TestErrorHandler *t)
  {
    return "TestErrorHandler at " + pqxx::to_string(ptrdiff_t(t));
  }
};
}


namespace
{
void test_process_notice_calls_errorhandler(connection_base &c)
{
  vector<TestErrorHandler *> dummy;
  TestErrorHandler handler(c, dummy);
  c.process_notice("Error!\n");
  PQXX_CHECK_EQUAL(handler.message, "Error!\n", "Error not handled.");
}


void test_error_handlers_get_called_newest_to_oldest(connection_base &c)
{
  vector<TestErrorHandler *> handlers;
  TestErrorHandler h1(c, handlers);
  TestErrorHandler h2(c, handlers);
  TestErrorHandler h3(c, handlers);
  c.process_notice("Warning.\n");
  PQXX_CHECK_EQUAL(h3.message, "Warning.\n", "Message not handled.");
  PQXX_CHECK_EQUAL(h2.message, "Warning.\n", "Broken handling chain.");
  PQXX_CHECK_EQUAL(h1.message, "Warning.\n", "Insane handling chain.");
  PQXX_CHECK_EQUAL(handlers.size(), 3u, "Wrong number of handler calls.");
  PQXX_CHECK_EQUAL(&h3, handlers[0], "Unexpected handling order.");
  PQXX_CHECK_EQUAL(&h2, handlers[1], "Insane handling order.");
  PQXX_CHECK_EQUAL(&h1, handlers[2], "Impossible handling order.");
}

void test_returning_false_stops_error_handling(connection_base &c)
{
  vector<TestErrorHandler *> handlers;
  TestErrorHandler starved(c, handlers);
  TestErrorHandler blocker(c, handlers, false);
  c.process_notice("Error output.\n");
  PQXX_CHECK_EQUAL(handlers.size(), 1u, "Handling chain was not stopped.");
  PQXX_CHECK_EQUAL(handlers[0], &blocker, "Wrong handler got message.");
  PQXX_CHECK_EQUAL(blocker.message, "Error output.\n", "Didn't get message.");
  PQXX_CHECK_EQUAL(starved.message, "", "Message received; it shouldn't be.");
}

void test_destroyed_error_handlers_are_not_called(connection_base &c)
{
  vector<TestErrorHandler *> handlers;
  {
    TestErrorHandler doomed(c, handlers);
  }
  c.process_notice("Unheard output.");
  PQXX_CHECK(handlers.empty(), "Message was received on dead errorhandler.");
}

void test_destroying_connection_unregisters_handlers()
{
  TestErrorHandler *survivor;
  vector<TestErrorHandler *> handlers;
  {
    connection c;
    survivor = new TestErrorHandler(c, handlers);
  }
  // Make some pointless use of survivor just to prove that this doesn't crash.
  (*survivor)("Hi");
  PQXX_CHECK_EQUAL(
	handlers.size(),
	1u,
	"Ghost of dead ex-connection haunts handler.");
}


void test_errorhandler(transaction_base &t)
{
  test_process_notice_calls_errorhandler(t.conn());
  test_error_handlers_get_called_newest_to_oldest(t.conn());
  test_returning_false_stops_error_handling(t.conn());
  test_destroyed_error_handlers_are_not_called(t.conn());
  test_destroying_connection_unregisters_handlers();
}
} // namespace

PQXX_REGISTER_TEST(test_errorhandler)
