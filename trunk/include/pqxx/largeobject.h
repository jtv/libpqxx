/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/largeobject.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "largeobject" instead (no ".h")
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_LARGEOBJECT_H
#define PQXX_LARGEOBJECT_H

#include "pqxx/util.h"
#include "pqxx/largeobject"

namespace pqxx
{

/// @deprecated For compatibility with old LargeObject class
typedef largeobject LargeObject;

/// @deprecated For compatibility with old LargeObjectAccess class
typedef largeobjectaccess LargeObjectAccess;

}

#endif

