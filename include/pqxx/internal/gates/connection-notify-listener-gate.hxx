namespace pqxx
{
class notify_listener;

namespace internal
{
class PQXX_PRIVATE connection_notify_listener_gate
{
  friend class pqxx::notify_listener;

  connection_notify_listener_gate(connection_base &home) : m_home(home) {}

  void add_listener(notify_listener *listener)
	{ m_home.add_listener(listener); }
  void remove_listener(notify_listener *listener) throw ()
	{ m_home.remove_listener(listener); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
