/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/notify-listen.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::notify_listener class.
 *   pqxx::notify_listener describes a notification to wait on, and what it does
 *
 * Copyright (c) 2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include <string>

#include "pqxx/notify-listen"

#include "pqxx/internal/gates/connection-notify-listener.hxx"

using namespace pqxx::internal;

pqxx::notify_listener::notify_listener(
	connection_base &c,
	const PGSTD::string &n) :
  m_conn(c),
  m_Name(n)
{
  gate::connection_notify_listener(c).add_listener(this);
}


pqxx::notify_listener::~notify_listener() throw ()
{
  connection_base &c = conn();
#ifdef PQXX_QUIET_DESTRUCTORS
  disable_noticer Quiet(c);
#endif
  gate::connection_notify_listener(c).remove_listener(this);
}
