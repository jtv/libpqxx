/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transactionitf.h
 *
 *   DESCRIPTION
 *      Backward compatibility for when Transaction_base was TransactionItf
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTIONITF_H
#define PQXX_TRANSACTIONITF_H

#include <pqxx/transaction_base.h>

namespace pqxx
{
/// @deprecated Legacy compatibility.  Use Transaction_base instead.
/** Transaction_base used to be called TransactionItf.  This was changed to 
 * convey its role in a way more consistent with the C++ standard library.
 */
typedef Transaction_base TransactionItf;
}

#endif

