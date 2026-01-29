#include <chrono>

#include <pqxx/internal/header-pre.hxx>

#include <pqxx/internal/wait.hxx>

#include <pqxx/internal/header-post.hxx>

#include <pqxx/nontransaction>
#include <pqxx/notification>
#include <pqxx/subtransaction>

#include "helpers.hxx"

namespace
{
#include <pqxx/internal/ignore-deprecated-pre.hxx>
struct TestReceiver final : public pqxx::notification_receiver
{
  TestReceiver(pqxx::connection &cx, std::string const &channel_name) :
          pqxx::notification_receiver(cx, channel_name)
  {}
  ~TestReceiver() override = default;

  void operator()(std::string const &payload_string, int backend) override
  {
    m_payload = payload_string;
    m_backend_pid = backend;
  }

  [[nodiscard]] std::string const &payload() const noexcept
  {
    return m_payload;
  }
  [[nodiscard]] int backend_pid() const noexcept { return m_backend_pid; }

  TestReceiver(TestReceiver const &) = delete;
  TestReceiver &operator=(TestReceiver const &) = delete;
  TestReceiver(TestReceiver &&) = delete;
  TestReceiver &operator=(TestReceiver &&) = delete;

private:
  std::string m_payload;
  int m_backend_pid = 0;
};


void test_receive_classic(
  pqxx::transaction_base &tx, std::string const &channel,
  char const payload[] = nullptr)
{
  pqxx::connection &cx(tx.conn());

  std::string SQL{"NOTIFY " + tx.quote_name(channel)};
  if (payload != nullptr)
    SQL += ", " + tx.quote(payload);

  TestReceiver const receiver{cx, channel};

  // Clear out any previously pending notifications that might otherwise
  // confuse the test.
  cx.get_notifs();

  // Notify, and receive.
  tx.exec(SQL);
  tx.commit();

  int notifs{0};
  for (int i{0}; (i < 100) and (notifs == 0);
       ++i, pqxx::internal::wait_for(100u))
    notifs = cx.get_notifs();

  PQXX_CHECK_EQUAL(notifs, 1);
  PQXX_CHECK_EQUAL(receiver.backend_pid(), cx.backendpid());
  if (payload == nullptr)
    PQXX_CHECK(std::empty(receiver.payload()));
  else
    PQXX_CHECK_EQUAL(receiver.payload(), payload);
}


void test_notification_classic(pqxx::test::context &tctx)
{
  pqxx::connection cx;

  std::string const chan0{tctx.make_name("pqxx-chan")},
    chan1{tctx.make_name("pqxx-chan")}, chan2{tctx.make_name("pqxx-chan")};
  TestReceiver const receiver(cx, chan0);
  PQXX_CHECK_EQUAL(receiver.channel(), chan0);

  pqxx::work tx{cx};
  test_receive_classic(tx, chan1);

  pqxx::nontransaction u(cx);
  test_receive_classic(u, chan2, "payload");
}
#include <pqxx/internal/ignore-deprecated-post.hxx>


void test_notification_to_self_arrives_after_commit(pqxx::test::context &tctx)
{
  pqxx::connection cx;

  auto const channel{tctx.make_name("pqxx-chan")};
  int notifications{0};
  pqxx::connection *conn{nullptr};
  std::string incoming, payload;
  int pid{0};

  cx.listen(
    channel,
    [&notifications, &conn, &incoming, &payload, &pid](pqxx::notification n) {
      ++notifications;
      conn = &n.conn;
      incoming = n.channel;
      pid = n.backend_pid;
      payload = n.payload;
    });

  cx.get_notifs();

  // No notifications so far.
  PQXX_CHECK_EQUAL(notifications, 0);

  pqxx::work tx{cx};
  tx.notify(channel);
  int received{cx.await_notification(0, 300)};

  // Notification has not been delivered yet, since transaction has not yet
  // been committed.
  PQXX_CHECK_EQUAL(received, 0, "Notification went out before commit.");
  PQXX_CHECK_EQUAL(notifications, 0);

  tx.commit();

  received = cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1);
  PQXX_CHECK_EQUAL(notifications, 1);
  PQXX_CHECK(conn == &cx);
  PQXX_CHECK_EQUAL(pid, cx.backendpid());
  PQXX_CHECK_EQUAL(incoming, channel);
  PQXX_CHECK_EQUAL(payload, "");
}


void test_notification_has_payload(pqxx::test::context &tctx)
{
  pqxx::connection cx;

  auto const channel{tctx.make_name("pqxx-chan")};
  auto const payload{"two dozen eggs"};
  int notifications{0};
  std::string received;

  cx.listen(channel, [&notifications, &received](pqxx::notification n) {
    ++notifications;
    received = n.payload;
  });

  pqxx::work tx{cx};
  tx.notify(channel, payload);
  tx.commit();

  cx.await_notification(3);

  PQXX_CHECK_EQUAL(notifications, 1);
  PQXX_CHECK_EQUAL(received, payload);
}


// Functor-shaped notification handler.
struct notify_test_listener final
{
  explicit notify_test_listener(int &r) : m_received{&r} {}
  void operator()(pqxx::notification) { ++*m_received; }

private:
  int *m_received;
};


void test_listen_supports_different_types_of_callable(
  pqxx::test::context &tctx)
{
  pqxx::connection cx;
  auto const chan{tctx.make_name("pqxx-chan")};
  int received{};

  // Using a functor as a handler.
  received = 0;
  notify_test_listener const l(received);
  cx.listen(chan, l);
  pqxx::work tx1{cx};
  tx1.notify(chan);
  tx1.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1);

  // Using a handler that takes a const reference to the notification.
  received = 0;
  cx.listen(chan, [&received](pqxx::notification const &) { ++received; });
  pqxx::work tx2{cx};
  tx2.notify(chan);
  tx2.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1);

  // Using a handler that takes an rvalue reference.
  received = 0;
  cx.listen(chan, [&received](pqxx::notification &&) { ++received; });
  pqxx::work tx3{cx};
  tx3.notify(chan);
  tx3.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1);
}


void test_abort_cancels_notification(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  cx.listen(chan, [&chan](pqxx::notification const &n) {
    throw pqxx::test::test_failure{std::format(
      "Got unexpected notification on channel '{}' (payload '{}').  "
      "(Was waiting for '{}'.)",
      n.channel.c_str(), n.payload.c_str(), chan)};
  });

  pqxx::work tx{cx};
  tx.notify(chan);
  tx.abort();

  // This will throw a test failure if the notification should unexpectedly
  // still arrive.
  cx.await_notification(3);
}


void test_notification_channels_are_case_sensitive(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  std::string in;
  auto const base{tctx.make_name("pqxx-chan")};
  cx.listen(base + "AbC", [&in](pqxx::notification n) { in = n.channel; });

  pqxx::work tx{cx};
  tx.notify(base + "AbC");
  tx.notify(base + "ABC");
  tx.notify(base + "abc");
  tx.commit();

  cx.await_notification(3);

  PQXX_CHECK_EQUAL(in, base + "AbC");
}


void test_notification_channels_may_contain_weird_chars(
  pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-A_#&*!")};
  pqxx::connection cx;
  std::string got;
  cx.listen(chan, [&got](pqxx::notification n) { got = n.channel; });
  pqxx::work tx{cx};
  tx.notify(chan);
  tx.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(got, chan);
}


/// In a nontransaction, a notification goes out even if you abort.
void test_nontransaction_sends_notification(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });

  pqxx::nontransaction tx{cx};
  tx.notify(chan);
  tx.abort();

  cx.await_notification(3);
  PQXX_CHECK(got);
}


void test_subtransaction_sends_notification(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });

  pqxx::work tx{cx};
  pqxx::subtransaction sx{tx};
  sx.notify(chan);
  sx.commit();
  tx.commit();

  cx.await_notification(3);
  PQXX_CHECK(got);
}


void test_subtransaction_abort_cancels_notification(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  cx.listen(chan, [&chan](pqxx::notification const &n) {
    throw pqxx::test::test_failure{std::format(
      "Got unexpected notification on channel '{}' (payload '{}').  (Was "
      "waiting for '{}'.)",
      n.channel.c_str(), n.payload.c_str(), chan)};
  });

  pqxx::work tx{cx};
  pqxx::subtransaction sx{tx};
  sx.notify(chan);
  sx.abort();
  tx.commit();

  // This will throw if we unexpectedly got the notification.
  cx.await_notification(3);
}


void test_cannot_listen_during_transaction(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  // Listening while a transaction is active is an error, even when it's just
  // a nontransaction.
  pqxx::nontransaction const tx{cx};
  PQXX_CHECK_THROWS(
    cx.listen(tctx.make_name("pqxx-chan"), [](pqxx::notification) {}),
    pqxx::usage_error);
}


void test_notifications_cross_connections(pqxx::test::context &tctx)
{
  pqxx::connection cx_listen, cx_notify;
  auto const chan{tctx.make_name("pqxx-chan")};
  int sender_pid{0};
  cx_listen.listen(
    chan, [&sender_pid](pqxx::notification n) { sender_pid = n.backend_pid; });

  pqxx::work tx{cx_notify};
  tx.notify(chan);
  tx.commit();

  cx_listen.await_notification(3);
  PQXX_CHECK_EQUAL(sender_pid, cx_notify.backendpid());
}


void test_notification_goes_to_right_handler(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  std::string got;
  int count{0};
  std::string const chanx{tctx.make_name("pqxx-chanX")},
    chany{tctx.make_name("pqxx-chanY")}, chanz{tctx.make_name("pqxx-chanZ")};

  cx.listen(chanx, [&got, &count](pqxx::notification) {
    got = "chanX";
    ++count;
  });
  cx.listen(chany, [&got, &count](pqxx::notification) {
    got = "chanY";
    ++count;
  });
  cx.listen(chanz, [&got, &count](pqxx::notification) {
    got = "chanZ";
    ++count;
  });

  pqxx::work tx{cx};
  tx.notify(chany);
  tx.commit();
  cx.await_notification(3);

  PQXX_CHECK_EQUAL(got, "chanY");
  PQXX_CHECK_EQUAL(count, 1);
}


void test_listen_on_same_channel_overwrites(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  std::string got;
  int count{0};

  cx.listen(chan, [&got, &count](pqxx::notification) {
    got = "first";
    ++count;
  });
  cx.listen(chan, [&got, &count](pqxx::notification) {
    got = "second";
    ++count;
  });
  cx.listen(chan, [&got, &count](pqxx::notification) {
    got = "third";
    ++count;
  });

  pqxx::work tx{cx};
  tx.notify(chan);
  tx.commit();
  cx.await_notification(3);

  PQXX_CHECK_EQUAL(count, 1, "Expected 1 notification despite overwrite.");
  PQXX_CHECK_EQUAL(got, "third");
}


void test_empty_notification_handler_disables(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });
  cx.listen(chan);
  pqxx::work tx{cx};
  tx.notify(chan);
  tx.commit();
  PQXX_CHECK(not got, "Disabling a notification handler did not work.");
}


void test_notifications_do_not_come_in_until_commit(pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });

  // This applies even during a nontransaction.  Another test verifies that
  // a notification goes _out_ even if we abort the nontransaction, because
  // it goes out immediately, not at commit time.  What we're establishing
  // here is that the notification does not come _in_ during a transaction,
  // even if it's a nontransaction.
  pqxx::nontransaction tx{cx};
  tx.notify(chan);
  cx.await_notification(3);
  PQXX_CHECK(not got, "Notification came in during nontransaction.");
}


void test_notification_handlers_follow_connection_move(
  pqxx::test::context &tctx)
{
  auto const chan{tctx.make_name("pqxx-chan")};
  pqxx::connection cx1;
  pqxx::connection *got{nullptr};
  cx1.listen(chan, [&got](pqxx::notification n) { got = &n.conn; });
  pqxx::connection cx2{std::move(cx1)};
  pqxx::connection cx3;
  cx3 = std::move(cx2);
  pqxx::work tx{cx3};
  tx.notify(chan);
  tx.commit();
  cx3.await_notification(3);
  PQXX_CHECK(got != nullptr);
  PQXX_CHECK(got == &cx3);
}


PQXX_REGISTER_TEST(test_notification_classic);
PQXX_REGISTER_TEST(test_notification_to_self_arrives_after_commit);
PQXX_REGISTER_TEST(test_notification_has_payload);
PQXX_REGISTER_TEST(test_listen_supports_different_types_of_callable);
PQXX_REGISTER_TEST(test_abort_cancels_notification);
PQXX_REGISTER_TEST(test_notification_channels_are_case_sensitive);
PQXX_REGISTER_TEST(test_notification_channels_may_contain_weird_chars);
PQXX_REGISTER_TEST(test_nontransaction_sends_notification);
PQXX_REGISTER_TEST(test_subtransaction_sends_notification);
PQXX_REGISTER_TEST(test_subtransaction_abort_cancels_notification);
PQXX_REGISTER_TEST(test_cannot_listen_during_transaction);
PQXX_REGISTER_TEST(test_notifications_cross_connections);
PQXX_REGISTER_TEST(test_notification_goes_to_right_handler);
PQXX_REGISTER_TEST(test_listen_on_same_channel_overwrites);
PQXX_REGISTER_TEST(test_empty_notification_handler_disables);
PQXX_REGISTER_TEST(test_notifications_do_not_come_in_until_commit);
PQXX_REGISTER_TEST(test_notification_handlers_follow_connection_move);
} // namespace
