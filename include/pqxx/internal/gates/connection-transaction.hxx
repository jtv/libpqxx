#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class connection;
}

namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_transaction : callgate<connection>
{
  friend class pqxx::transaction_base;

  connection_transaction(reference x) : super(x) {}

  result exec(const char query[]) { return home().exec(query); }
  void register_transaction(transaction_base *t)
	{ home().register_transaction(t); }
  void unregister_transaction(transaction_base *t) noexcept
	{ home().unregister_transaction(t); }

  bool read_copy_line(std::string &line)
	{ return home().read_copy_line(line); }
  void write_copy_line(std::string_view line)
	{ home().write_copy_line(line); }
  void end_copy_write() { home().end_copy_write(); }

  result exec_prepared(
	const std::string &statement,
	const internal::params &args)
  {
    return home().exec_prepared(statement, args);
  }

  result exec_params(const std::string &query, const internal::params &args)
  {
    return home().exec_params(query, args);
  }
};
} // namespace pqxx::internal::gate
