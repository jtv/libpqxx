#include <chrono>

#include <pqxx/internal/header-pre.hxx>

#include <pqxx/internal/wait.hxx>

#include <pqxx/internal/header-post.hxx>

#include <pqxx/nontransaction>
#include <pqxx/notification>

#include "../test_helpers.hxx"

namespace
{
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


void test_receive(
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


void test_notification()
{
  pqxx::connection cx;
  TestReceiver receiver(cx, "mychannel");
  PQXX_CHECK_EQUAL(receiver.channel(), "mychannel", "Bad channel.");

  pqxx::work tx{cx};
  test_receive(tx, "channel1");

  pqxx::nontransaction u(cx);
  test_receive(u, "channel2", "payload");
}


PQXX_REGISTER_TEST(test_notification);
} // namespace
