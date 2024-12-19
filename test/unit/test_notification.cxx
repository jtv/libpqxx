#include <chrono>

#include <pqxx/internal/header-pre.hxx>

#include <pqxx/internal/wait.hxx>

#include <pqxx/internal/header-post.hxx>

#include <pqxx/nontransaction>
#include <pqxx/notification>

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

// XXX: different kinds of callable, including different signatures.
// XXX: nontransaction.
// XXX: subtransaction.
// XXX: refuses to listen() while in a transaction.
// XXX: send across connections.
// XXX: Choosing (just) the right one out of multiple handlers.


PQXX_REGISTER_TEST(test_notification_classic);
PQXX_REGISTER_TEST(test_notification_to_self_arrives_after_commit);
PQXX_REGISTER_TEST(test_notification_has_payload);
} // namespace
