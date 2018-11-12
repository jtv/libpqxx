/** Implementation of the pqxx::tablestream2 class.
 *
 * pqxx::tablestream2 provides optimized batch access to a database table.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/tablestream2.hxx"
#include "pqxx/transaction"


pqxx::tablestream2::tablestream2(transaction_base &tb) :
  internal::namedclass("tablestream2"),
  internal::transactionfocus(tb),
  m_finished{false}
{}


pqxx::tablestream2::~tablestream2() noexcept
{}


pqxx::tablestream2::operator bool() const noexcept
{
  return !m_finished;
}


bool pqxx::tablestream2::operator!() const noexcept
{
  return !static_cast<bool>(*this);
}


void pqxx::tablestream2::close()
{
  if (*this)
  {
    m_finished = true;
    unregister_me();
  }
}
