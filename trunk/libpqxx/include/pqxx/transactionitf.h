/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transactionitf.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "transaction_base" instead
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTIONITF_H
#define PQXX_TRANSACTIONITF_H

#include "pqxx/transaction_base"

namespace pqxx
{
/// @deprecated Legacy compatibility.  Use transaction_base instead.
/** What is now transaction_base used to be called TransactionItf.  This was 
 * changed to convey its role in a way more consistent with the C++ standard 
 * library.
 */
typedef transaction_base TransactionItf;
}

#endif

