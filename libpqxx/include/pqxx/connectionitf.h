/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connectionitf.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "connection_base" instead.
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CONNECTIONITF_H
#define PQXX_CONNECTIONITF_H

#include "pqxx/util.h"
#include "pqxx/connection_base"

namespace pqxx
{
/// @deprecated Legacy compatibility.  Use connection_base instead.
/** What is now connection_base used to be called ConnectionItf.  This was 
 * changed to convey its role in a way more consistent with the C++ standard 
 * library.
 */
typedef connection_base ConnectionItf;
}

#endif
