/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/notify-listen.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::notify_listener functor interface.
 *   pqxx::notify_listener describes a notification to wait on, and what it does
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/notify-listen instead.
 *
 * Copyright (c) 2001-2007, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/connection_base"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{
/// "Observer" base class for notifications.
/** @addtogroup notification Notifications and Listeners
 * To listen on a notification issued using the NOTIFY command, derive your own
 * class from notify_listener and define its function call operator to perform
 * whatever action you wish to take when the given notification arrives.  Then
 * create an object of that class and pass it to your connection.  DO NOT use
 * raw SQL to listen for notifications, or your attempts to listen won't be
 * resumed when a connection fails--and you'll have no way to notice.
 *
 * Notifications never arrive inside a backend transaction.  Therefore, unless
 * you may be using a nontransaction when a notification arrives, you are free
 * to open a transaction of your own inside your listener's function invocation
 * operator.
 *
 * Notifications you are listening for may arrive anywhere within libpqxx code,
 * but be aware that @b PostgreSQL @b defers @b notifications @b occurring
 * @b inside @b transactions.  (This was done for excellent reasons; just think
 * about what happens if the transaction where you happen to handle an incoming
 * notification is later rolled back for other reasons).  So if you're keeping a
 * transaction open, don't expect any of your listeners on the same connection
 * to be notified.
 *
 * Multiple listeners on the same connection may listen on a notification of the
 * same name.  An incoming notification is processed by invoking all listeners
 * (zero or more) of the same name.
 */
class PQXX_LIBEXPORT notify_listener : public PGSTD::unary_function<int, void>
{
public:
  /// Constructor.  Registers the listener with connection C.
  /**
   * @param C Connection this listener resides in.
   * @param N Name of the notification to listen for.
   */
  notify_listener(connection_base &C, const PGSTD::string &N) :		//[t4]
    m_Conn(C), m_Name(N) { m_Conn.add_listener(this); }

  virtual ~notify_listener() throw ()					//[t4]
  {
#ifdef PQXX_QUIET_DESTRUCTORS
    disable_noticer Quiet(Conn());
#endif
    m_Conn.remove_listener(this);
  }

  const PGSTD::string &name() const { return m_Name; }			//[t4]

  /// Overridable: action to invoke when notification arrives
  /**
   * @param be_pid Process ID of the database backend process that served our
   * connection when the notification arrived.  The actual process ID behind the
   * connection may have changed by the time this method is called.
   */
  virtual void operator()(int be_pid) =0;				//[t4]


protected:
  connection_base &Conn() const throw () { return m_Conn; }		//[t23]

private:
  /// Not allowed
  notify_listener(const notify_listener &);
  /// Not allowed
  notify_listener &operator=(const notify_listener &);

  connection_base &m_Conn;
  PGSTD::string m_Name;
};

}


#include "pqxx/compiler-internal-post.hxx" 

