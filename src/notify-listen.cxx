/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/notify-listen.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::notify_listener class.
 *   Predecessor to notification_receiver.  Deprecated.  Do not use.
 *
 * Copyright (c) 2009-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <functional>
#include <string>

#include "pqxx/notify-listen"


void pqxx::internal::notify_listener_forwarder::operator()(
	const PGSTD::string &,
	int backend_pid)
{
  (*m_wrappee)(backend_pid);
}


pqxx::notify_listener::notify_listener(
	connection_base &c,
	const PGSTD::string &n) :
  m_conn(c),
  m_forwarder(c, n, this)
{
}


pqxx::notify_listener::~notify_listener() throw ()
{
}
