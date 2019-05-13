/** Implementation of the pqxx::dbtransaction class.
 *
 * pqxx::dbtransaction represents a real backend transaction.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/dbtransaction"


namespace
{
std::string generate_set_transaction(
	pqxx::readwrite_policy rw,
	std::string_view isolation)
{
  const bool set_isolation = (
	not isolation.empty() and
	isolation!= pqxx::isolation_traits<pqxx::read_committed>::name());
  const bool read_only = (rw != pqxx::read_write);

  if (not read_only and not set_isolation) return "BEGIN";

  static const std::string_view prefix = "BEGIN; SET TRANSACTION ";

  std::string args;
  args.reserve(prefix.size() + 30 + isolation.size());
  args.append(prefix);
  if (set_isolation)
  {
    args.append(" ISOLATION LEVEL ");
    args.append(isolation);
  }
  if (read_only) args.append(" READ ONLY");
  return args;
}
} // namespace


pqxx::dbtransaction::dbtransaction(
	connection &C,
	std::string_view isolation,
	readwrite_policy rw) :
  namedclass{"dbtransaction"},
  transaction_base{C},
  m_start_cmd{generate_set_transaction(rw, isolation)}
{
}


pqxx::dbtransaction::dbtransaction(
	connection &C,
	bool direct,
	readwrite_policy rw) :
  namedclass{"dbtransaction"},
  transaction_base(C, direct),
  m_start_cmd{generate_set_transaction(rw, "")}
{
}


pqxx::dbtransaction::~dbtransaction()
{
}


void pqxx::dbtransaction::do_begin()
{
  direct_exec(m_start_cmd.c_str());
}


pqxx::result pqxx::dbtransaction::do_exec(const char Query[])
{
  try
  {
    return direct_exec(Query);
  }
  catch (const std::exception &)
  {
    try { abort(); } catch (const std::exception &) {}
    throw;
  }
}


void pqxx::dbtransaction::do_abort()
{
  direct_exec("ROLLBACK");
}


std::string pqxx::dbtransaction::fullname(
	std::string_view ttype,
	std::string_view isolation)
{
  std::string name;
  name.reserve(ttype.size() + isolation.size() + 2);
  name.push_back('<');
  name.append(ttype);
  name.append(isolation);
  name.push_back('>');
  return name;
}
