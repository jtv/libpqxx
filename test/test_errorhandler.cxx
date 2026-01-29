#include <vector>

#include <pqxx/connection>
#include <pqxx/errorhandler>

#include "helpers.hxx"

namespace
{
class TestErrorHandler final : public pqxx::errorhandler
{
public:
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  TestErrorHandler(
    pqxx::connection &cx, std::vector<TestErrorHandler *> &activated_handlers,
    bool retval = true) :
          pqxx::errorhandler(cx),
          m_handler_list(activated_handlers),
          m_return_value(retval)
  {}
#include "pqxx/internal/ignore-deprecated-post.hxx"

  bool operator()(char const msg[]) noexcept override
  {
    m_message = std::string{msg};
    m_handler_list.push_back(this);
    return m_return_value;
  }

  [[nodiscard]] std::string message() const { return m_message; }

  TestErrorHandler() = delete;
  TestErrorHandler(TestErrorHandler const &) = delete;
  TestErrorHandler &operator=(TestErrorHandler const &) = delete;
  TestErrorHandler(TestErrorHandler &&) = delete;
  TestErrorHandler &operator=(TestErrorHandler &&) = delete;
  ~TestErrorHandler() override = default;

private:
  std::vector<TestErrorHandler *> &m_handler_list;
  std::string m_message;
  bool m_return_value;
};
} // namespace


namespace pqxx
{
template<> struct nullness<TestErrorHandler *> final
{
  // clang warns about these being unused.  And clang 6 won't accept a
  // [[maybe_unused]] attribute on them either!

  [[maybe_unused]] static inline constexpr bool has_null{true};
  static inline constexpr bool always_null{false};

  static constexpr bool is_null(TestErrorHandler *e) noexcept
  {
    return e == nullptr;
  }
  static constexpr TestErrorHandler *null() noexcept { return nullptr; }
};


template<> struct string_traits<TestErrorHandler *> final
{
  static constexpr std::size_t size_buffer(TestErrorHandler *const &) noexcept
  {
    return 100;
  }

  static std::string_view
  to_buf(std::span<char> buf, TestErrorHandler *const &value, ctx c = {})
  {
    auto const sz{std::format_to_n(
                    std::data(buf), std::ssize(buf), "TestErrorHandler at {}",
                    std::size_t(value))
                    .size};
    if (std::cmp_greater_equal(sz, std::size(buf)))
      throw conversion_overrun{
        "Not enough buffer for TestErrorHandler.", c.loc};
    return {std::data(buf), static_cast<std::size_t>(sz)};
  }
};
} // namespace pqxx


namespace
{
void test_process_notice_calls_errorhandler(pqxx::connection &cx)
{
  std::vector<TestErrorHandler *> dummy;
  TestErrorHandler const handler(cx, dummy);
  cx.process_notice("Error!\n");
  PQXX_CHECK_EQUAL(handler.message(), "Error!\n");
}


void test_error_handlers_get_called_newest_to_oldest(pqxx::connection &cx)
{
  std::vector<TestErrorHandler *> handlers;
  TestErrorHandler h1(cx, handlers);
  TestErrorHandler h2(cx, handlers);
  TestErrorHandler h3(cx, handlers);
  cx.process_notice("Warning.\n");
  PQXX_CHECK_EQUAL(h3.message(), "Warning.\n");
  PQXX_CHECK_EQUAL(h2.message(), "Warning.\n");
  PQXX_CHECK_EQUAL(h1.message(), "Warning.\n");
  PQXX_CHECK_EQUAL(std::size(handlers), 3u);
  PQXX_CHECK_EQUAL(&h3, handlers.at(0));
  PQXX_CHECK_EQUAL(&h2, handlers.at(1));
  PQXX_CHECK_EQUAL(&h1, handlers.at(2));
}

void test_returning_false_stops_error_handling(pqxx::connection &cx)
{
  std::vector<TestErrorHandler *> handlers;
  TestErrorHandler const starved(cx, handlers);
  TestErrorHandler blocker(cx, handlers, false);
  cx.process_notice("Error output.\n");
  PQXX_CHECK_EQUAL(std::size(handlers), 1u, "Handling chain was not stopped.");
  PQXX_CHECK_EQUAL(handlers.at(0), &blocker, "Wrong handler got message.");
  PQXX_CHECK_EQUAL(blocker.message(), "Error output.\n");
  PQXX_CHECK_EQUAL(
    starved.message(), "", "Message received; it shouldn't be.");
}

void test_destroyed_error_handlers_are_not_called(pqxx::connection &cx)
{
  std::vector<TestErrorHandler *> handlers;
  {
    TestErrorHandler const doomed(cx, handlers);
  }
  cx.process_notice("Unheard output.");
  PQXX_CHECK(
    std::empty(handlers), "Message was received on dead errorhandler.");
}

class MinimalErrorHandler final : public pqxx::errorhandler
{
public:
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  explicit MinimalErrorHandler(pqxx::connection &cx) : pqxx::errorhandler(cx)
  {}
#include "pqxx/internal/ignore-deprecated-post.hxx"
  bool operator()(char const[]) noexcept override { return true; }

  ~MinimalErrorHandler() override = default;
  MinimalErrorHandler(MinimalErrorHandler const &) = delete;
  MinimalErrorHandler &operator=(MinimalErrorHandler const &) = delete;
  MinimalErrorHandler(MinimalErrorHandler &&) = delete;
  MinimalErrorHandler &operator=(MinimalErrorHandler &&) = delete;
};


void test_get_errorhandlers(pqxx::connection &cx)
{
  std::unique_ptr<MinimalErrorHandler> eh3;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  auto const handlers_before{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
  std::size_t const base_handlers{std::size(handlers_before)};

  {
    MinimalErrorHandler eh1(cx);
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    auto const handlers_with_eh1{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_EQUAL(std::size(handlers_with_eh1), base_handlers + 1);
    PQXX_CHECK_EQUAL(
      std::size_t(*std::rbegin(handlers_with_eh1)), std::size_t(&eh1));

    {
      MinimalErrorHandler eh2(cx);
#include "pqxx/internal/ignore-deprecated-pre.hxx"
      auto const handlers_with_eh2{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
      PQXX_CHECK_EQUAL(std::size(handlers_with_eh2), base_handlers + 2);
      PQXX_CHECK_EQUAL(
        std::size_t(*(std::rbegin(handlers_with_eh2) + 1)), std::size_t(&eh1));
      PQXX_CHECK_EQUAL(
        std::size_t(*std::rbegin(handlers_with_eh2)), std::size_t(&eh2));
    }
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    auto const handlers_without_eh2{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_EQUAL(std::size(handlers_without_eh2), base_handlers + 1);
    PQXX_CHECK_EQUAL(
      std::size_t(*std::rbegin(handlers_without_eh2)), std::size_t(&eh1));

    eh3 = std::make_unique<MinimalErrorHandler>(cx);
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    auto const handlers_with_eh3{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_EQUAL(std::size(handlers_with_eh3), base_handlers + 2);
    PQXX_CHECK_EQUAL(
      std::size_t(*std::rbegin(handlers_with_eh3)), std::size_t(eh3.get()));
  }
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  auto const handlers_without_eh1{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_EQUAL(std::size(handlers_without_eh1), base_handlers + 1);
  PQXX_CHECK_EQUAL(
    std::size_t(*std::rbegin(handlers_without_eh1)), std::size_t(eh3.get()));

  eh3.reset();

#include "pqxx/internal/ignore-deprecated-pre.hxx"
  auto const handlers_without_all{cx.get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_EQUAL(std::size(handlers_without_all), base_handlers);
}


void test_errorhandler(pqxx::test::context &)
{
  pqxx::connection cx;
  test_process_notice_calls_errorhandler(cx);
  test_error_handlers_get_called_newest_to_oldest(cx);
  test_returning_false_stops_error_handling(cx);
  test_destroyed_error_handlers_are_not_called(cx);
  test_get_errorhandlers(cx);
}


PQXX_REGISTER_TEST(test_errorhandler);
} // namespace
