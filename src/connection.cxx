/** Implementation of the pqxx::connection and sibling classes.
 *
 * Different ways of setting up a backend connection.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include <stdexcept>

extern "C"
{
#include "libpq-fe.h"
}

#include "pqxx/connection"


pqxx::connectionpolicy::handle
pqxx::connectionpolicy::normalconnect(handle orig)
{
  if (orig) return orig;
  orig = PQconnectdb(m_options.c_str());
  if (orig == nullptr) throw std::bad_alloc{};
  if (PQstatus(orig) != CONNECTION_OK)
  {
    const std::string msg{PQerrorMessage(orig)};
    PQfinish(orig);
    throw broken_connection{msg};
  }
  return orig;
}


pqxx::connectionpolicy::handle
pqxx::connectionpolicy::do_startconnect(handle orig)
{
  if (orig) return orig;
  orig = normalconnect(orig);
  if (PQstatus(orig) != CONNECTION_OK)
  {
    const std::string msg{PQerrorMessage(orig)};
    do_disconnect(orig);
    throw broken_connection{msg};
  }
  return orig;
}

pqxx::connectionpolicy::handle
pqxx::connectionpolicy::do_disconnect(handle orig) noexcept
{
  if (orig) PQfinish(orig);
  return nullptr;
}


bool pqxx::connectionpolicy::is_ready(handle h) const noexcept
{
  return h != nullptr;
}
