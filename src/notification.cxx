/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/notification.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::notification_receiever class.
 *   pqxx::notification_receiver processes notifications.
 *
 * Copyright (c) 2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <string>

#include "pqxx/internal/gates/connection-notification_receiver.hxx"

#include "pqxx/notification"

#ifdef PQXX_QUIET_DESTRUCTORS
#include "pqxx/errorhandler"
#endif


using namespace pqxx::internal;


pqxx::notification_receiver::notification_receiver(
	connection_base &c,
	const PGSTD::string &channel_name) :
  m_conn(c),
  m_channel(channel_name)
{
  gate::connection_notification_receiver(c).add_receiver(this);
}


pqxx::notification_receiver::~notification_receiver()
{
#ifdef PQXX_QUIET_DESTRUCTORS
  quiet_errorhandler quiet(this->conn());
#endif
  gate::connection_notification_receiver(this->conn()).remove_receiver(this);
}
