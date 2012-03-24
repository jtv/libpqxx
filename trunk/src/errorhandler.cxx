/*-------------------------------------------------------------------------
 *
 *   FILE
 *	errorhandler.cxx
 *
 *   DESCRIPTION
 *      implementation of pqxx::errorhandler and helpers.
 *   pqxx::errorhandler allows programs to receive errors and warnings.
 *
 * Copyright (c) 2011-2012, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/connection_base"
#include "pqxx/errorhandler"

#include "pqxx/internal/gates/connection-errorhandler.hxx"


using namespace pqxx;
using namespace pqxx::internal;


pqxx::errorhandler::errorhandler(connection_base &conn) :
  m_home(&conn)
{
  gate::connection_errorhandler(*m_home).register_errorhandler(this);
}


pqxx::errorhandler::~errorhandler()
{
  unregister();
}


void pqxx::errorhandler::unregister() PQXX_NOEXCEPT
{
  if (m_home)
  {
    gate::connection_errorhandler connection_gate(*m_home);
    m_home = NULL;
    connection_gate.unregister_errorhandler(this);
  }
}
