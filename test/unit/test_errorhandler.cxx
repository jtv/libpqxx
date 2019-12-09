#include <vector>

#include "../test_helpers.hxx"

namespace
{
class TestErrorHandler final : public pqxx::errorhandler
{
public:
  TestErrorHandler(
	pqxx::connection_base &c,
	std::vector<TestErrorHandler *> &activated_handlers,
	bool retval=true) :
    pqxx::errorhandler(c),
    return_value(retval),
    message(),
    handler_list(activated_handlers)
  {}

  bool operator()(const char msg[]) noexcept override
  {
    message = std::string{msg};
    handler_list.push_back(this);
    return return_value;
  }

  bool return_value;
  std::string message;
  std::vector<TestErrorHandler *> &handler_list;
};
} // namespace


namespace pqxx
{
template<> struct nullness<TestErrorHandler *>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(TestErrorHandler *e) noexcept
	{ return e == nullptr; }
  static constexpr TestErrorHandler *null() noexcept { return nullptr; }
};


template<> class str<TestErrorHandler *>
{
public:
  explicit str(TestErrorHandler *t) :
    m_str("TestErrorHandler at " + pqxx::to_string(ptrdiff_t(t)))
  {}

  operator std::string_view() const noexcept { return m_str; }
  std::string_view view() const noexcept { return m_str; }
  const char *c_str() const noexcept { return m_str.c_str(); }

private:
  std::string m_str;
};


template<> struct string_traits<TestErrorHandler *>
{
  static constexpr size_t size_buffer(TestErrorHandler *const &) noexcept
  { return 100; }

  static char *into_buf(char *begin, char *end, TestErrorHandler *const &value)
  {
    std::string text{"TestErrorHandler at " + pqxx::to_string(value)};
    if (static_cast<ptrdiff_t>(text.size()) >= (end - begin))
      throw conversion_overrun{"Not enough buffer for TestErrorHandler."};
    std::memcpy(begin, text.c_str(), text.size() + 1);
    return begin + text.size() + 1;
  }
};
} // namespace pqxx


namespace
{
void test_process_notice_calls_errorhandler(pqxx::connection_base &c)
{
  std::vector<TestErrorHandler *> dummy;
  TestErrorHandler handler(c, dummy);
  c.process_notice("Error!\n");
  PQXX_CHECK_EQUAL(handler.message, "Error!\n", "Error not handled.");
}


void test_error_handlers_get_called_newest_to_oldest(pqxx::connection_base &c)
{
  std::vector<TestErrorHandler *> handlers;
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

void test_returning_false_stops_error_handling(pqxx::connection_base &c)
{
  std::vector<TestErrorHandler *> handlers;
  TestErrorHandler starved(c, handlers);
  TestErrorHandler blocker(c, handlers, false);
  c.process_notice("Error output.\n");
  PQXX_CHECK_EQUAL(handlers.size(), 1u, "Handling chain was not stopped.");
  PQXX_CHECK_EQUAL(handlers[0], &blocker, "Wrong handler got message.");
  PQXX_CHECK_EQUAL(blocker.message, "Error output.\n", "Didn't get message.");
  PQXX_CHECK_EQUAL(starved.message, "", "Message received; it shouldn't be.");
}

void test_destroyed_error_handlers_are_not_called(pqxx::connection_base &c)
{
  std::vector<TestErrorHandler *> handlers;
  {
    TestErrorHandler doomed(c, handlers);
  }
  c.process_notice("Unheard output.");
  PQXX_CHECK(handlers.empty(), "Message was received on dead errorhandler.");
}

void test_destroying_connection_unregisters_handlers()
{
  TestErrorHandler *survivor;
  std::vector<TestErrorHandler *> handlers;
  {
    pqxx::connection c;
    survivor = new TestErrorHandler(c, handlers);
  }
  // Make some pointless use of survivor just to prove that this doesn't crash.
  (*survivor)("Hi");
  PQXX_CHECK_EQUAL(
	handlers.size(),
	1u,
	"Ghost of dead ex-connection haunts handler.");
  delete survivor;
}


class MinimalErrorHandler final : public pqxx::errorhandler
{
public:
  explicit MinimalErrorHandler(pqxx::connection_base &c) :
	pqxx::errorhandler(c) {}
  bool operator()(const char[]) noexcept override { return true; }
};


void test_get_errorhandlers(pqxx::connection_base &c)
{
  MinimalErrorHandler *eh3 = nullptr;
  const auto handlers_before = c.get_errorhandlers();
  const size_t base_handlers = handlers_before.size();

  {
    MinimalErrorHandler eh1(c);
    const auto handlers_with_eh1 = c.get_errorhandlers();
    PQXX_CHECK_EQUAL(
	handlers_with_eh1.size(),
	base_handlers + 1,
	"Registering a handler didn't create exactly one handler.");
    PQXX_CHECK_EQUAL(
	size_t(*handlers_with_eh1.rbegin()),
	size_t(&eh1),
	"Wrong handler or wrong order.");

    {
      MinimalErrorHandler eh2(c);
      const auto handlers_with_eh2 = c.get_errorhandlers();
      PQXX_CHECK_EQUAL(
	handlers_with_eh2.size(),
	base_handlers + 2,
	"Adding second handler didn't work.");
      PQXX_CHECK_EQUAL(
	size_t(*(handlers_with_eh2.rbegin() + 1)),
	size_t(&eh1),
	"Second handler upset order.");
      PQXX_CHECK_EQUAL(
	size_t(*handlers_with_eh2.rbegin()),
	size_t(&eh2), "Second handler isn't right.");
    }
    const auto handlers_without_eh2 = c.get_errorhandlers();
    PQXX_CHECK_EQUAL(
	handlers_without_eh2.size(),
	base_handlers + 1,
	"Handler destruction produced wrong-sized handlers list.");
    PQXX_CHECK_EQUAL(
	size_t(*handlers_without_eh2.rbegin()),
	size_t(&eh1),
	"Destroyed wrong handler.");

    eh3 = new MinimalErrorHandler(c);
    const auto handlers_with_eh3 = c.get_errorhandlers();
    PQXX_CHECK_EQUAL(
	handlers_with_eh3.size(),
	base_handlers + 2,
	"Remove-and-add breaks.");
    PQXX_CHECK_EQUAL(
	size_t(*handlers_with_eh3.rbegin()),
	size_t(eh3),
	"Added wrong third handler.");
  }
  const auto handlers_without_eh1 = c.get_errorhandlers();
  PQXX_CHECK_EQUAL(
	handlers_without_eh1.size(),
	base_handlers + 1,
	"Destroying oldest handler didn't work as expected.");
  PQXX_CHECK_EQUAL(
	size_t(*handlers_without_eh1.rbegin()),
	size_t(eh3),
	"Destroyed wrong handler.");

  delete eh3;

  const auto handlers_without_all = c.get_errorhandlers();
  PQXX_CHECK_EQUAL(
	handlers_without_all.size(),
	base_handlers,
	"Destroying all custom handlers didn't work as expected.");
}


void test_errorhandler()
{
  pqxx::connection conn;
  test_process_notice_calls_errorhandler(conn);
  test_error_handlers_get_called_newest_to_oldest(conn);
  test_returning_false_stops_error_handling(conn);
  test_destroyed_error_handlers_are_not_called(conn);
  test_destroying_connection_unregisters_handlers();
  test_get_errorhandlers(conn);
}


PQXX_REGISTER_TEST(test_errorhandler);
} // namespace
