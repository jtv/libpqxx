/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/trigger.h
 *
 *   DESCRIPTION
 *      definition of the Pg::Trigger functor interface.
 *   Pg::Trigger describes a database trigger to wait on, and what it does
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_TRIGGER_H
#define PG_TRIGGER_H

#include <string>

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace Pg
{

// To listen on a database trigger, derive your own class from Trigger and
// define its function call operator to perform whatever action you wish to
// take when the given trigger arrives.  Then create an object of that class
// and pass it to your Connection.  DO NOT set triggers directly through SQL,
// or they won't be restored when your connection fails--and you'll have no
// way to notice.
//
// Trigger notifications never arrive inside a transaction.  Therefore, you
// are free to open a transaction of your own inside your Trigger's function
// invocation operator.
//
// Notifications for your trigger may arrive anywhere within libpqxx code, but
// be aware that POSTGRESQL DEFERS NOTIFICATIONS OCCURRING INSIDE TRANSACTIONS.
// So if you're keeping a transaction open, don't expect any of your Triggers
// on the same connection to be notified.
class Trigger
{
public:
  Trigger(Connection &C, PGSTD::string N) : 				//[t4]
    m_Conn(C), m_Name(N) 
  { 
    m_Conn.AddTrigger(this); 
  }

  virtual ~Trigger() { m_Conn.RemoveTrigger(this); }			//[t4]

  PGSTD::string Name() const { return m_Name; }				//[t4]

  virtual void operator()(int be_pid) =0;				//[t4]

protected:
  Connection &Conn() const { return m_Conn; }				//[t4]

private:
  Connection &m_Conn;
  PGSTD::string m_Name;
};

}


#endif

