/*-------------------------------------------------------------------------
 *
 *   FILE
 *	nontransaction.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::NonTransaction class.
 *   Pg::Transaction provides nontransactional database access
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */

#include "pqxx/nontransaction.h"


using namespace PGSTD;

Pg::NonTransaction::~NonTransaction()
{
  End();
}


Pg::Result Pg::NonTransaction::DoExec(const char C[])
{
  return DirectExec(C, 2, 0);
}

