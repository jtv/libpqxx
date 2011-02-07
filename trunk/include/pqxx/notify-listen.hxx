/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/notify-listen.hxx
 *
 *   DESCRIPTION
 *      Definition of the obsolete pqxx::notify_listener functor interface.
 *   Predecessor to notification_receiver.  Deprecated.  Do not use.
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/notify-listen instead.
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_NOTIFY_LISTEN
#define PQXX_H_NOTIFY_LISTEN

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/notification"


namespace pqxx
{
class connection_base;
class notify_listener;

namespace internal
{
/// Internal helper class to support old-style, payloadless notifications.
class notify_listener_forwarder: public notification_receiver
{
public:
  notify_listener_forwarder(
	connection_base &c,
	const PGSTD::string &channel_name,
	notify_listener *wrappee) :
    notification_receiver(c, channel_name),
    m_wrappee(wrappee)
  {}

  virtual void operator()(const PGSTD::string &, int backend_pid);

private:
  notify_listener *m_wrappee;
};
}


/// Obsolete notification receiver.
/** @deprecated Use notification_receiver instead.
 */
class PQXX_LIBEXPORT PQXX_NOVTABLE notify_listener :
  public PGSTD::unary_function<int, void>
{
public:
  notify_listener(connection_base &c, const PGSTD::string &n);
  virtual ~notify_listener() throw ();
  const PGSTD::string &name() const { return m_forwarder.channel(); }
  virtual void operator()(int be_pid) =0;


protected:
  connection_base &Conn() const throw () { return conn(); }
  connection_base &conn() const throw () { return m_conn; }

private:
  connection_base &m_conn;
  internal::notify_listener_forwarder m_forwarder;
};
}


#include "pqxx/compiler-internal-post.hxx" 

#endif
