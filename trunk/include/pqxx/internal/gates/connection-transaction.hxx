#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class connection_base;

namespace internal
{
namespace gate
{
class PQXX_PRIVATE connection_transaction : callgate<connection_base>
{
  friend class pqxx::transaction_base;

  connection_transaction(reference x) : super(x) {}

  result Exec(const char query[], int retries)
	{ return home().Exec(query, retries); }
  void RegisterTransaction(transaction_base *t)
	{ home().RegisterTransaction(t); }
  void UnregisterTransaction(transaction_base *t) PQXX_NOEXCEPT
	{ home().UnregisterTransaction(t); }

  bool ReadCopyLine(std::string &line)
	{ return home().ReadCopyLine(line); }
  void WriteCopyLine(const std::string &line)
	{ home().WriteCopyLine(line); }
  void EndCopyWrite() { home().EndCopyWrite(); }

  std::string RawGetVar(const std::string &var)
	{ return home().RawGetVar(var); }
  void RawSetVar(const std::string &var, const std::string &value)
	{ home().RawSetVar(var, value); }
  void AddVariables(const std::map<std::string, std::string> &vars)
	{ home().AddVariables(vars); }

  result prepared_exec(
	const std::string &statement,
	const char *const params[],
	const int paramlengths[],
	const int binaries[],
	int nparams)
  {
    return home().prepared_exec(
	statement,
	params,
	paramlengths,
	binaries,
	nparams);
  }

  bool prepared_exists(const std::string &statement) const
	{ return home().prepared_exists(statement); }

  void take_reactivation_avoidance(int counter)
	{ home().m_reactivation_avoidance.add(counter); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
