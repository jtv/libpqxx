/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/subtransaction.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::subtransaction class.
 *   pqxx::subtransaction is a nested transaction, i.e. one within a transaction
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/subtransaction instead.
 *
 * Copyright (c) 2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include "pqxx/dbtransaction"



/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{

/**
 * @addtogroup transaction Transaction classes
 */
//@{

class PQXX_LIBEXPORT subtransaction :
  public internal::transactionfocus,
  public dbtransaction
{
public:
  explicit subtransaction(dbtransaction &T,
	const PGSTD::string &Name=PGSTD::string());			//[t88]

private:
  virtual void do_begin();						//[t88]
  virtual void do_commit();						//[t88]
  virtual void do_abort();						//[t88]

  dbtransaction &m_parent;
};

//@}

}


