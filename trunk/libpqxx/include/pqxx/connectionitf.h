/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connectionitf.h
 *
 *   DESCRIPTION
 *      Backward compatibility for when Connection_base was still ConnectionItf
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CONNECTIONITF_H
#define PQXX_CONNECTIONITF_H

#include <pqxx/connection_base.h>

namespace pqxx
{
/// @deprecated Legacy compatibility.  Use Connection_base instead.
/** Connection_base used to be called ConnectionItf.  This was changed to 
 * convey its role in a way more consistent with the C++ standard library.
 */
typedef Connection_base ConnectionItf;
}

#endif
