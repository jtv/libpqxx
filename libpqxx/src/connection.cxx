/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::connection and pqxx::lazyconnection classes.
 *   Different ways of setting up a backend connection.
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <stdexcept>

#ifdef PQXX_HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif	// PQXX_HAVE_SYS_SELECT_H

#include "pqxx/connection"

using namespace PGSTD;

pqxx::connection::connection() :
  connection_base(0)
{
  startconnect();
}

pqxx::connection::connection(const PGSTD::string &ConnInfo) :
  connection_base(ConnInfo)
{
  startconnect();
}

pqxx::connection::connection(const char ConnInfo[]) :
  connection_base(ConnInfo)
{
  startconnect();
}

// Work around problem with Sun CC 5.1
pqxx::connection::~connection()
{
}


void pqxx::connection::startconnect()
{
  if (!get_conn()) set_conn(PQconnectdb(options()));
}


void pqxx::connection::completeconnect()
{
  if (!get_conn()) throw broken_connection();
}


pqxx::lazyconnection::lazyconnection() :
  connection_base(0)
{
  startconnect();
}


pqxx::lazyconnection::lazyconnection(const string &ConnInfo) :
  connection_base(ConnInfo)
{
  startconnect();
}


pqxx::lazyconnection::lazyconnection(const char ConnInfo[]) :
  connection_base(ConnInfo)
{
  startconnect();
}


// Work around problem with Sun CC 5.1
pqxx::lazyconnection::~lazyconnection()
{
}


// Work around problem with Sun CC 5.1
pqxx::asyncconnection::~asyncconnection()
{
}


void pqxx::lazyconnection::completeconnect()
{
  if (!get_conn()) set_conn(PQconnectdb(options()));
  if (!is_open()) throw broken_connection();
}


pqxx::asyncconnection::asyncconnection() :
  connection_base(0),
  m_connecting(false)
{
  startconnect();
}


pqxx::asyncconnection::asyncconnection(const string &ConnInfo) :
  connection_base(ConnInfo),
  m_connecting(false)
{
  startconnect();
}


pqxx::asyncconnection::asyncconnection(const char ConnInfo[]) :
  connection_base(ConnInfo),
  m_connecting(false)
{
  startconnect();
}


void pqxx::asyncconnection::startconnect()
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

  const int fd = PQsocket(get_conn());
  if (fd < 0) throw broken_connection();

  PostgresPollingStatusType pollstatus;
  fd_set fds;
  FD_ZERO(&fds);
#ifdef PQXX_SELECT_ACCEPTS_NULL
  fd_set *const none = 0;
#else
  fd_set fdnone;
  FD_ZERO(&fdnone);
  fd_set *const none = &fdnone;
#endif

  do
  {
    pollstatus = PQconnectPoll(get_conn());
    switch (pollstatus)
    {
    case PGRES_POLLING_FAILED:
      throw broken_connection();

    case PGRES_POLLING_READING:
      FD_SET(fd, &fds);
      select(fd+1, &fds, none, &fds, 0);
      break;

    case PGRES_POLLING_WRITING:
      FD_SET(fd, &fds);
      select(fd+1, none, &fds, &fds, 0);
      break;

    case PGRES_POLLING_ACTIVE:
    case PGRES_POLLING_OK:
      break;
    }
  } while (pollstatus != PGRES_POLLING_OK);
}


