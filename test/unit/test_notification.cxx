#include "../test_helpers.hxx"

using namespace pqxx;


namespace
{
class TestReceiver final : public notification_receiver
{
public:
  std::string payload;
  int backend_pid;

  TestReceiver(connection_base &c, const std::string &channel_name) :
    notification_receiver(c, channel_name),
    payload(),
    backend_pid(0)
  {
  }

  virtual void operator()(const std::string &payload_string, int backend)
	override
  {
    this->payload = payload_string;
    this->backend_pid = backend;
  }
};


void test_receive(
	transaction_base &t,
	const std::string &channel,
	const char payload[] = nullptr)
{
  connection_base &conn(t.conn());

  std::string SQL = "NOTIFY \"" + channel + "\"";
  if (payload) SQL += ", " + t.quote(payload);

  TestReceiver receiver(t.conn(), channel);

  // Clear out any previously pending notifications that might otherwise
  // confuse the test.
  conn.get_notifs();

  // Notify, and receive.
  t.exec(SQL);
  t.commit();

  int notifs = 0;
  for (
	int i=0;
	(i < 10) and (notifs == 0);
	++i, pqxx::internal::sleep_seconds(1))
    notifs = conn.get_notifs();

  PQXX_CHECK_EQUAL(notifs, 1, "Got wrong number of notifications.");
  PQXX_CHECK_EQUAL(receiver.backend_pid, conn.backendpid(), "Bad pid.");
  if (payload) PQXX_CHECK_EQUAL(receiver.payload, payload, "Bad payload.");
  else PQXX_CHECK(receiver.payload.empty(), "Unexpected payload.");
}


void test_notification()
{
  connection conn;
  TestReceiver receiver(conn, "mychannel");
  PQXX_CHECK_EQUAL(receiver.channel(), "mychannel", "Bad channel.");

  work tx{conn};
  test_receive(tx, "channel1");

  nontransaction u(conn);
  test_receive(u, "channel2", "payload");
}


PQXX_REGISTER_TEST(test_notification);
} // namespace
