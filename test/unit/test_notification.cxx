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
    [&notifications, &conn, &incoming, &payload, &pid](pqxx::notification n){
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
    pid, cx.backendpid(), "Notification from self came from wrong connection.");
  PQXX_CHECK_EQUAL(incoming, channel, "Notification is on wrong channel.");
  PQXX_CHECK_EQUAL(payload, "", "Unexpected payload.");
}


void test_notification_has_payload()
{
  pqxx::connection cx;

  auto const channel{"pqxx-ichan"}, payload{"two dozen eggs"};
  int notifications{0};
  std::string received;

  cx.listen(
    channel,
    [&notifications, &received](pqxx::notification n) {
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
  cx.listen(chan, [&received](pqxx::notification){ received = true; });

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
  cx.listen("pqxx-AbC", [&in](pqxx::notification n){ in = n.channel; });

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
  cx.listen(chan, [&got](pqxx::notification n){ got = n.channel; });
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
  cx.listen(chan, [&got](pqxx::notification){ got = true; });

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
  cx.listen(chan, [&got](pqxx::notification){ got = true; });

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
  cx.listen(chan, [&got](pqxx::notification){ got = true; });

  pqxx::work tx{cx};
  pqxx::subtransaction sx{tx};
  sx.notify(chan);
  sx.abort();
  tx.commit();

  cx.await_notification(3);
  PQXX_CHECK(not got, "Subtransaction rollback did not cancel notification.");
}

// XXX: refuses to listen() while in a transaction.
// XXX: send across connections.
// XXX: Choosing (just) the right one out of multiple handlers.
// XXX: overwriting
// XXX: notifications go _out_ immediately duruing nontransaction
// XXX: notifications do not come in during even a nontransaction


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
} // namespace
