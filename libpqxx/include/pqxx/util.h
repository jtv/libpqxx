/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/util.h
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_UTIL_H
#define PQXX_UTIL_H

#if defined(PQXX_HAVE_CPP_WARNING)
#warning "Deprecated libpqxx header included.  Use headers without '.h'"
#elif defined(PQXX_HAVE_CPP_PRAGMA_MESSAGE)
#pragma message("Deprecated libpqxx header included.  Use headers without '.h'")
#endif

#define PQXX_DEPRECATED_HEADERS
#include "pqxx/util"

#endif

