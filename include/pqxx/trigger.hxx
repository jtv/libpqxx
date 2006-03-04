/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/trigger.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::trigger functor interface.
 *   pqxx::trigger describes a database trigger to wait on, and what it does
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/trigger instead.
 *
 * Copyright (c) 2001-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
/// "Observer" base class for trigger notifications.
/** @addtogroup notification Notifications and Triggers
 * To listen on a database trigger, derive your own class from trigger and
 * define its function call operator to perform whatever action you wish to
 * take when the given trigger arrives.  Then create an object of that class
 * and pass it to your connection.  DO NOT set triggers directly through SQL,
 * or they won't be restored when a connection fails--and you'll have no way to
 * notice.
 *
 * Trigger notifications never arrive inside a backend transaction.  Therefore,
 * unless you may be using a nontransaction when a notification arrives, you are
 * free to open a transaction of your own inside your trigger's function
 * invocation operator.
 *
 * Notifications for your trigger may arrive anywhere within libpqxx code, but
 * be aware that @b PostgreSQL @b defers @b notifications @b occurring @b inside
 * @b transactions.  (This was done for excellent reasons; just think about what
 * happens if the transaction where you happen to handle an incoming
 * notification is later rolled back for other reasons).  So if you're keeping a
 * transaction open, don't expect any of your triggers on the same connection
 * to be notified.
 *
 * Multiple triggers on the same connection may have the same name.  An incoming
 * notification is processed by invoking all triggers (zero or more) of the same
 * name.
 */
class PQXX_LIBEXPORT trigger : public PGSTD::unary_function<int, void>
{
public:
  /// Constructor.  Registers the trigger with connection C.
  /**
   * @param C Connection this trigger resides in.
   * @param N A name for the trigger.
   */
  trigger(connection_base &C, const PGSTD::string &N) : 		//[t4]
    m_Conn(C), m_Name(N) { m_Conn.AddTrigger(this); }

  virtual ~trigger() throw () 						//[t4]
  {
#ifdef PQXX_QUIET_DESTRUCTORS
    internal::disable_noticer Quiet(Conn());
#endif
    m_Conn.RemoveTrigger(this);
  }

  const PGSTD::string &name() const { return m_Name; }			//[t4]

  /// Overridable: action to invoke when trigger is notified.
  /**
   * @param be_pid Process ID of the database backend process that served our
   * connection when the trigger was notified.  The actual process ID behind the
   * connection may have changed by the time this method is called.
   */
  virtual void operator()(int be_pid) =0;				//[t4]


#ifdef PQXX_DEPRECATED_HEADERS
  /// @deprecated Use name() instead
  PGSTD::string Name() const { return name(); }
#endif

protected:
  connection_base &Conn() const throw () { return m_Conn; }		//[t23]

private:
  connection_base &m_Conn;
  PGSTD::string m_Name;
};

}


#include "pqxx/compiler-internal-post.hxx"
