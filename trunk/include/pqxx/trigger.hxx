/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/trigger.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::trigger functor interface.
 *   OBSOLETE.  Include pqxx/notify-listen instead.
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_TRIGGER
#define PQXX_H_TRIGGER

#include "pqxx/notify-listen"

namespace pqxx
{
/**
 * @deprecated The trigger class from libpqxx 1.x/2.x has been
 * replaced with notification_receiver.
 */
typedef notify_listener trigger;
}

#endif

