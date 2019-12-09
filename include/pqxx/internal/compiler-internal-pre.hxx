/* Compiler deficiency workarounds for compiling libpqxx headers.
 *
 * To be called at the start of each libpqxx header, in order to push the
 * client program's settings and apply libpqxx's settings.
 *
 * Must be balanced by an include of -header-post.hxx at the end
 * of the header.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
// NO GUARD HERE! This code should be included every time this file is.

#if defined(_MSC_VER)

// Save compiler's warning state, and set warning level 4.
// Setting the warning level explicitly ensures that libpqxx
// headers will compiler at this warning level as well.
#pragma warning (push,4)

#pragma warning (disable: 4511) // Copy constructor could not be generated.
#pragma warning (disable: 4512) // Assignment operator could not be generated.

// XXX: Resolve this workaround.
#pragma warning (disable: 4251) // Can't use standard library stuff in library.
#pragma warning (disable: 4275) // Can't inherit from non-exported class.

#endif // _MSC_VER
