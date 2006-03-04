/*-------------------------------------------------------------------------
 *
 *   FILE
 *     pqxx/compiler-internal-post.hxx
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for compiling libpqxx headers.
 *      To be called at the end of each libpqxx header, in order to
 *      restore the client program's settings.
 *
 * Copyright (c) 2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
// NO GUARDS HERE! This code should be executed every time!

#ifdef _WIN32

#ifdef _MSC_VER
#pragma warning (pop) // Restore client program's warning state
#endif

#endif

