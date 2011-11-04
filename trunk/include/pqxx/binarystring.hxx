/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/binarystring.hxx
 *
 *   DESCRIPTION
 *      Representation for raw, binary data.
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/binarystring instead.
 *
 * Copyright (c) 2003-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_BINARYSTRING
#define PQXX_H_BINARYSTRING

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <string>

#include "pqxx/result"


namespace pqxx
{

/// Binary data corresponding to PostgreSQL's "BYTEA" binary-string type.
/** @addtogroup escaping String escaping
 *
 * This class represents a binary string as stored in a field of type bytea.
 * The raw value returned by a bytea field contains escape sequences for certain
 * characters, which are filtered out by binarystring.
 *
 * Internally a binarystring is zero-terminated, but it may also contain zero
 * bytes, just like any other byte value.  So don't assume that it can be
 * treated as a C-style string unless you've made sure of this yourself.
 *
 * The binarystring retains its value even if the result it was obtained from is
 * destroyed, but it cannot be copied or assigned.
 *
 * \relatesalso transaction_base::esc_raw
 *
 * To convert the other way, i.e. from a raw series of bytes to a string
 * suitable for inclusion as bytea values in your SQL, use the transaction's
 * esc_raw() functions.
 *
 * @warning This class is implemented as a reference-counting smart pointer.
 * Copying, swapping, and destroying binarystring objects that refer to the same
 * underlying data block is <em>not thread-safe</em>.  If you wish to pass
 * binarystrings around between threads, make sure that each of these operations
 * is protected against concurrency with similar operations on the same object,
 * or other objects pointing to the same data block.
 */
class PQXX_LIBEXPORT binarystring :
	internal::PQAlloc<
		const unsigned char,
		pqxx::internal::freemallocmem_templated<const unsigned char> >
{
public:
  typedef content_type char_type;
  typedef PGSTD::char_traits<char_type>::char_type value_type;
  typedef size_t size_type;
  typedef long difference_type;
  typedef const value_type &const_reference;
  typedef const value_type *const_pointer;
  typedef const_pointer const_iterator;

#ifdef PQXX_HAVE_REVERSE_ITERATOR
  typedef PGSTD::reverse_iterator<const_iterator> const_reverse_iterator;
#endif

private:
  typedef internal::PQAlloc<
	value_type,
	pqxx::internal::freemallocmem_templated<const unsigned char> >
    super;

public:
  /// Read and unescape bytea field
  /** The field will be zero-terminated, even if the original bytea field isn't.
   * @param F the field to read; must be a bytea field
   */
  explicit binarystring(const field &);					//[t62]

  /// Copy binary data from std::string.
  explicit binarystring(const PGSTD::string &);

  /// Copy binary data of given length straight out of memory.
  binarystring(const void *, size_t);

  /// Size of converted string in bytes
  size_type size() const throw () { return m_size; }			//[t62]
  /// Size of converted string in bytes
  size_type length() const throw () { return size(); }			//[t62]
  bool empty() const throw () { return size()==0; }			//[t62]

  const_iterator begin() const throw () { return data(); }		//[t62]
  const_iterator end() const throw () { return data()+m_size; }		//[t62]

  const_reference front() const throw () { return *begin(); }		//[t62]
  const_reference back() const throw () { return *(data()+m_size-1); }	//[t62]

#ifdef PQXX_HAVE_REVERSE_ITERATOR
  const_reverse_iterator rbegin() const					//[t62]
	{ return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const					//[t62]
	{ return const_reverse_iterator(begin()); }
#endif

  /// Unescaped field contents
  const value_type *data() const throw () {return super::get();}	//[t62]

  const_reference operator[](size_type i) const throw ()		//[t62]
	{ return data()[i]; }

  bool PQXX_PURE operator==(const binarystring &) const throw ();	//[t62]
  bool operator!=(const binarystring &rhs) const throw ()		//[t62]
	{ return !operator==(rhs); }

  /// Index contained string, checking for valid index
  const_reference at(size_type) const;					//[t62]

  /// Swap contents with other binarystring
  void swap(binarystring &);						//[t62]

  /// Raw character buffer (no terminating zero is added)
  /** @warning No terminating zero is added!  If the binary data did not end in
   * a null character, you will not find one here.
   */
  const char *get() const throw ()					//[t62]
  {
    return reinterpret_cast<const char *>(super::get());
  }

  /// Read as regular C++ string (may include null characters)
  /** @warning libpqxx releases before 3.1 stored the string and returned a
   * reference to it.  This is no longer the case!  It now creates and returns
   * a new string object.  Avoid repeated use of this function; retrieve your
   * string once and keep it in a local variable.  Also, do not expect to be
   * able to compare the string's address to that of an earlier invocation.
   */
  PGSTD::string str() const;						//[t62]

private:
  size_type m_size;
};


/**
 * @addtogroup escaping String escaping
 *
 * @{
 */

/// Escape binary string for inclusion in SQL
/**
 * @deprecated Use the transaction's esc_raw() functions instead
 * \relatesalso binarystring
 */
PGSTD::string PQXX_LIBEXPORT escape_binary(const PGSTD::string &bin);
/// Escape binary string for inclusion in SQL
/**
 * @deprecated Use the transaction's esc_raw() functions instead
 * \relatesalso binarystring
 */
PGSTD::string PQXX_LIBEXPORT escape_binary(const char bin[]);
/// Escape binary string for inclusion in SQL
/**
 * @deprecated Use the transaction's esc_raw() functions instead
 * \relatesalso binarystring
 */
PGSTD::string PQXX_LIBEXPORT escape_binary(const char bin[], size_t len);
/// Escape binary string for inclusion in SQL
/**
 * @deprecated Use the transaction's esc_raw() functions instead
 * \relatesalso binarystring
 */
PGSTD::string PQXX_LIBEXPORT escape_binary(const unsigned char bin[]);
/// Escape binary string for inclusion in SQL
/**
 * @deprecated Use the transaction's esc_raw() functions instead
 * \relatesalso binarystring
 */
PGSTD::string PQXX_LIBEXPORT escape_binary(const unsigned char bin[], size_t len);

/**
 * @}
 */


}

#include "pqxx/compiler-internal-post.hxx"

#endif

