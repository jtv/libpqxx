#include <test_helpers.hxx>

using namespace std;
using namespace pqxx;


namespace
{
class TestReceiver PQXX_FINAL : public notification_receiver
{
public:
  string payload;
  int backend_pid;
  bool done;

  TestReceiver(connection_base &c, const string &channel_name) :
    notification_receiver(c, channel_name),
    payload(),
    backend_pid(0)
  {
  }

  virtual void operator()(const string &payload_string, int backend)
    PQXX_OVERRIDE
  {
    this->payload = payload_string;
    this->backend_pid = backend;
  }
};


void test_receive(
	transaction_base &t,
	const string &channel,
	const char payload[] = NULL)
{
  connection_base &conn(t.conn());

  string SQL = "NOTIFY \"" + channel + "\"";
  if (payload) SQL += ", " + t.quote(payload);

  TestReceiver receiver(t.conn(), channel);

  // Clear out any previously pending notifications that might otherwise
  // confuse the test.
  conn.get_notifs();

  // Notify, and receive.
  t.exec(SQL);
  t.commit();

  int notifs = 0;
  for (int i=0; (i < 10) && !notifs; ++i, pqxx::internal::sleep_seconds(1))
    notifs = conn.get_notifs();

  PQXX_CHECK_EQUAL(notifs, 1, "Got wrong number of notifications.");
  PQXX_CHECK_EQUAL(receiver.backend_pid, conn.backendpid(), "Bad pid.");
  if (payload) PQXX_CHECK_EQUAL(receiver.payload, payload, "Bad payload.");
  else PQXX_CHECK(receiver.payload.empty(), "Unexpected payload.");
}


void test_notification(transaction_base &t)
{
  connection_base &conn(t.conn());
  TestReceiver receiver(conn, "mychannel");
  PQXX_CHECK_EQUAL(receiver.channel(), "mychannel", "Bad channel.");

  test_receive(t, "channel1");

  nontransaction u(conn);
  test_receive(u, "channel2", "payload");
}
} // namespace

PQXX_REGISTER_TEST_T(test_notification, nontransaction)
