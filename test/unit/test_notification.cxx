#include <chrono>

#include <pqxx/internal/header-pre.hxx>

#include <pqxx/internal/wait.hxx>

#include <pqxx/internal/header-post.hxx>

#include <pqxx/nontransaction>
#include <pqxx/notification>
#include <pqxx/subtransaction>

#include "../test_helpers.hxx"

namespace
{
#include <pqxx/internal/ignore-deprecated-pre.hxx>
class TestReceiver final : public pqxx::notification_receiver
{
public:
  std::string payload;
  int backend_pid;

  TestReceiver(pqxx::connection &cx, std::string const &channel_name) :
          pqxx::notification_receiver(cx, channel_name),
          payload(),
          backend_pid(0)
  {}

  virtual void
  operator()(std::string const &payload_string, int backend) override
  {
    this->payload = payload_string;
    this->backend_pid = backend;
  }
};


void test_receive_classic(
  pqxx::transaction_base &tx, std::string const &channel,
  char const payload[] = nullptr)
{
  pqxx::connection &cx(tx.conn());

  std::string SQL{"NOTIFY " + tx.quote_name(channel)};
  if (payload != nullptr)
    SQL += ", " + tx.quote(payload);

  TestReceiver receiver{cx, channel};

  // Clear out any previously pending notifications that might otherwise
  // confuse the test.
  cx.get_notifs();

  // Notify, and receive.
  tx.exec(SQL);
  tx.commit();

  int notifs{0};
  for (int i{0}; (i < 10) and (notifs == 0);
       ++i, pqxx::internal::wait_for(1000u))
    notifs = cx.get_notifs();

  PQXX_CHECK_EQUAL(notifs, 1, "Got wrong number of notifications.");
  PQXX_CHECK_EQUAL(receiver.backend_pid, cx.backendpid(), "Bad pid.");
  if (payload == nullptr)
    PQXX_CHECK(std::empty(receiver.payload), "Unexpected payload.");
  else
    PQXX_CHECK_EQUAL(receiver.payload, payload, "Bad payload.");
}


void test_notification_classic()
{
  pqxx::connection cx;
  TestReceiver receiver(cx, "mychannel");
  PQXX_CHECK_EQUAL(receiver.channel(), "mychannel", "Bad channel.");

  pqxx::work tx{cx};
  test_receive_classic(tx, "channel1");

  pqxx::nontransaction u(cx);
  test_receive_classic(u, "channel2", "payload");
}
#include <pqxx/internal/ignore-deprecated-post.hxx>


void test_notification_to_self_arrives_after_commit()
{
  pqxx::connection cx;

  auto const channel{"pqxx_test_channel"};
  int notifications{0};
  pqxx::connection *conn;
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
  PQXX_CHECK_EQUAL(notifications, 0, "Unexpected notification.");

  pqxx::work tx{cx};
  tx.notify(channel);
  int received{cx.await_notification(0, 300)};

  // Notification has not been delivered yet, since transaction has not yet
  // been committed.
  PQXX_CHECK_EQUAL(received, 0, "Notification went out before commit.");
  PQXX_CHECK_EQUAL(notifications, 0, "Received uncounted notification.");

  tx.commit();

  received = cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1, "Did not receive 1 notification from self.");
  PQXX_CHECK_EQUAL(notifications, 1, "Miscounted notifcations.");
  PQXX_CHECK(conn == &cx, "Wrong connection on notification from self.");
  PQXX_CHECK_EQUAL(
    pid, cx.backendpid(),
    "Notification from self came from wrong connection.");
  PQXX_CHECK_EQUAL(incoming, channel, "Notification is on wrong channel.");
  PQXX_CHECK_EQUAL(payload, "", "Unexpected payload.");
}


void test_notification_has_payload()
{
  pqxx::connection cx;

  auto const channel{"pqxx-ichan"}, payload{"two dozen eggs"};
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

  PQXX_CHECK_EQUAL(notifications, 1, "Expeccted 1 self-notification.");
  PQXX_CHECK_EQUAL(received, payload, "Unexpected payload.");
}


// Functor-shaped notification handler.
struct notify_test_listener
{
  int &received;
  notify_test_listener(int &r) : received{r} {}
  void operator()(pqxx::notification) { ++received; }
};


void test_listen_supports_different_types_of_callable()
{
  auto const chan{"pqxx-test-listen"};
  pqxx::connection cx;
  int received;

  // Using a functor as a handler.
  received = 0;
  notify_test_listener l(received);
  cx.listen(chan, l);
  pqxx::work tx1{cx};
  tx1.notify(chan);
  tx1.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1, "Notification did not arrive.");

  // Using a handler that takes a const reference to the notification.
  received = 0;
  cx.listen(chan, [&received](pqxx::notification const &) { ++received; });
  pqxx::work tx2{cx};
  tx2.notify(chan);
  tx2.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1, "Const ref did not receive notification.");

  // Using a handler that takes an rvalue reference.
  received = 0;
  cx.listen(chan, [&received](pqxx::notification &&) { ++received; });
  pqxx::work tx3{cx};
  tx3.notify(chan);
  tx3.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(received, 1, "Revalue ref did not receive notification.");
}


void test_abort_cancels_notification()
{
  auto const chan{"pqxx-test-channel"};
  pqxx::connection cx;
  bool received{false};
  cx.listen(chan, [&received](pqxx::notification) { received = true; });

  pqxx::work tx{cx};
  tx.notify(chan);
  tx.abort();

  cx.await_notification(3);
  PQXX_CHECK(not received, "Abort did not cancel notification.");
}


void test_notification_channels_are_case_sensitive()
{
  pqxx::connection cx;
  std::string in;
  cx.listen("pqxx-AbC", [&in](pqxx::notification n) { in = n.channel; });

  pqxx::work tx{cx};
  tx.notify("pqxx-AbC");
  tx.notify("pqxx-ABC");
  tx.notify("pqxx-abc");
  tx.commit();

  cx.await_notification(3);

  PQXX_CHECK_EQUAL(in, "pqxx-AbC", "Channel is not case-insensitive.");
}


void test_notification_channels_may_contain_weird_chars()
{
  auto const chan{"pqxx-A_#&*!"};
  pqxx::connection cx;
  std::string got;
  cx.listen(chan, [&got](pqxx::notification n) { got = n.channel; });
  pqxx::work tx{cx};
  tx.notify(chan);
  tx.commit();
  cx.await_notification(3);
  PQXX_CHECK_EQUAL(
    got, chan, "Channel name with weird characters got distorted.");
}


/// In a nontransaction, a notification goes out even if you abort.
void test_nontransaction_sends_notification()
{
  auto const chan{"pqxx-test-chan"};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });

  pqxx::nontransaction tx{cx};
  tx.notify(chan);
  tx.abort();

  cx.await_notification(3);
  PQXX_CHECK(got, "Notification from nontransaction did not arrive.");
}


void test_subtransaction_sends_notification()
{
  auto const chan{"pqxx-test-chan6301"};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });

  pqxx::work tx{cx};
  pqxx::subtransaction sx{tx};
  sx.notify(chan);
  sx.commit();
  tx.commit();

  cx.await_notification(3);
  PQXX_CHECK(got, "Notification from subtransaction did not arrive.");
}


void test_subtransaction_abort_cancels_notification()
{
  auto const chan{"pqxx-test-chan123278w"};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });

  pqxx::work tx{cx};
  pqxx::subtransaction sx{tx};
  sx.notify(chan);
  sx.abort();
  tx.commit();

  cx.await_notification(3);
  PQXX_CHECK(not got, "Subtransaction rollback did not cancel notification.");
}


void test_cannot_listen_during_transaction()
{
  pqxx::connection cx;
  // Listening while a transaction is active is an error, even when it's just
  // a nontransaction.
  pqxx::nontransaction tx{cx};
  PQXX_CHECK_THROWS(
    cx.listen("pqxx-test-chan02756", [](pqxx::notification) {}),
    pqxx::usage_error,
    "Expected usage_error when listening during transaction.");
}


void test_notifications_cross_connections()
{
  auto const chan{"pqxx-chan7529"};
  pqxx::connection cx_listen, cx_notify;
  int sender_pid{0};
  cx_listen.listen(
    chan, [&sender_pid](pqxx::notification n) { sender_pid = n.backend_pid; });

  pqxx::work tx{cx_notify};
  tx.notify(chan);
  tx.commit();

  cx_listen.await_notification(3);
  PQXX_CHECK_EQUAL(sender_pid, cx_notify.backendpid(), "Sender pid mismatch.");
}


void test_notification_goes_to_right_handler()
{
  pqxx::connection cx;
  std::string got;
  int count{0};

  cx.listen("pqxx-chanX", [&got, &count](pqxx::notification) {
    got = "chanX";
    ++count;
  });
  cx.listen("pqxx-chanY", [&got, &count](pqxx::notification) {
    got = "chanY";
    ++count;
  });
  cx.listen("pqxx-chanZ", [&got, &count](pqxx::notification) {
    got = "chanZ";
    ++count;
  });

  pqxx::work tx{cx};
  tx.notify("pqxx-chanY");
  tx.commit();
  cx.await_notification(3);

  PQXX_CHECK_EQUAL(got, "chanY", "Wrong handler got called.");
  PQXX_CHECK_EQUAL(count, 1, "Wrong number of handler calls.");
}


void test_listen_on_same_channel_overwrites()
{
  auto const chan{"pqxx-chan84710"};
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
  PQXX_CHECK_EQUAL(got, "third", "Wrong handler called.");
}


void test_empty_notification_handler_disables()
{
  auto const chan{"pqxx-chan812710"};
  pqxx::connection cx;
  bool got{false};
  cx.listen(chan, [&got](pqxx::notification) { got = true; });
  cx.listen(chan);
  pqxx::work tx{cx};
  tx.notify(chan);
  tx.commit();
  PQXX_CHECK(not got, "Disabling a notification handler did not work.");
}


void test_notifications_do_not_come_in_until_commit()
{
  auto const chan{"pqxx-chan95017834"};
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


void test_notification_handlers_follow_connection_move()
{
  auto const chan{"pqxx-chan3782"};
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
  PQXX_CHECK(got != nullptr, "Did not get notified.");
  PQXX_CHECK(got == &cx3, "Notification got the wrong connection.");
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
