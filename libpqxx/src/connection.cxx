/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::connection and pqxx::lazyconnection classes.
 *   Different ways of setting up a backend connection.
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <stdexcept>

#include "pqxx/connection"

using namespace PGSTD;
using namespace pqxx::internal::pq;

// TODO: Existing structure doesn't call SetupState()!

pqxx::connection::connection() :
  connection_base(0)
{
  do_startconnect();
}

pqxx::connection::connection(const string &ConnInfo) :
  connection_base(ConnInfo)
{
  do_startconnect();
}

pqxx::connection::connection(const char ConnInfo[]) :
  connection_base(ConnInfo)
{
  do_startconnect();
}

// Conflicting workarounds for Windows and SUN CC 5.1; see header
#ifndef _WIN32
pqxx::connection::~connection() throw () { close(); }
pqxx::lazyconnection::~lazyconnection() throw () { close(); }
pqxx::asyncconnection::~asyncconnection() throw () {do_dropconnect(); close();}
#endif	// _WIN32


pqxx::asyncconnection::asyncconnection() :
  connection_base(0),
  m_connecting(false)
{
  do_startconnect();
}


pqxx::asyncconnection::asyncconnection(const string &ConnInfo) :
  connection_base(ConnInfo),
  m_connecting(false)
{
  do_startconnect();
}


pqxx::asyncconnection::asyncconnection(const char ConnInfo[]) :
  connection_base(ConnInfo),
  m_connecting(false)
{
  do_startconnect();
}


void pqxx::asyncconnection::do_startconnect()
{
  if (get_conn()) return;	// Already connecting or connected
  m_connecting = false;
  set_conn(PQconnectStart(options()));
  if (!get_conn()) throw bad_alloc();
  if (PQconnectPoll(get_conn()) == PGRES_POLLING_FAILED)
    throw broken_connection();
  m_connecting = true;
}

void pqxx::asyncconnection::completeconnect()
{
  if (!get_conn()) startconnect();
  if (!m_connecting) return;

  // Our "attempt to connect" state ends here, for better or for worse
  m_connecting = false;

  if (!get_conn()) throw broken_connection();

  PostgresPollingStatusType pollstatus;

  do
  {
    pollstatus = PQconnectPoll(get_conn());
    switch (pollstatus)
    {
    case PGRES_POLLING_FAILED:
      throw broken_connection();

    case PGRES_POLLING_READING:
      wait_read();
      break;

    case PGRES_POLLING_WRITING:
      wait_write();
      break;

    case PGRES_POLLING_ACTIVE:
    case PGRES_POLLING_OK:
      break;
    }
  } while (pollstatus != PGRES_POLLING_OK);
}


pqxx::nullconnection::~nullconnection() throw ()
{
}

