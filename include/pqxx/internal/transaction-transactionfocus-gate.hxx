namespace pqxx
{
namespace internal
{
class transactionfocus;

class PQXX_PRIVATE transaction_transactionfocus_gate
{
  friend class transactionfocus;

  transaction_transactionfocus_gate(transaction_base &home) : m_home(home) {}

  void RegisterFocus(transactionfocus *focus) { m_home.RegisterFocus(focus); }
  void UnregisterFocus(transactionfocus *focus) throw ()
	{ m_home.UnregisterFocus(focus); }
  void RegisterPendingError(const PGSTD::string &error)
	{ m_home.RegisterPendingError(error); }

  transaction_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
