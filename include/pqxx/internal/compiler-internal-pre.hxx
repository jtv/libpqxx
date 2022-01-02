/* Compiler deficiency workarounds for compiling libpqxx headers.
 *
 * To be called at the start of each libpqxx header, in order to push the
 * client program's settings and apply libpqxx's settings.
 *
 * Must be balanced by an include of -header-post.hxx at the end
 * of the header.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
// NO GUARD HERE! This code should be included every time this file is.

#if defined(_MSC_VER)

// Save compiler's warning state, and set warning level 4.
// Setting the warning level explicitly ensures that libpqxx
// headers will compiler at this warning level as well.
#  pragma warning(push, 4)

// Visual C++ generates some entirely unreasonable warnings.  Disable them.
// Copy constructor could not be generated.
#  pragma warning(disable : 4511)
// Assignment operator could not be generated.
#  pragma warning(disable : 4512)
// Can't expose outside classes without exporting them.  Except the MSVC docs
// say please ignore the warning if it's a standard library class.
#  pragma warning(disable : 4251)
// Can't derive library classes from outside classes without exporting them.
// Except the MSVC docs say please ignore the warning if the parent class is
// in the standard library.
#  pragma warning(disable : 4275)
// Can't inherit from non-exported class.
#  pragma warning(disable : 4275)

#endif // _MSC_VER
