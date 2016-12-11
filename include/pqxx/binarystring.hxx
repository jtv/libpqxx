/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/binarystring.hxx
 *
 *   DESCRIPTION
 *      Representation for raw, binary data.
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/binarystring instead.
 *
 * Copyright (c) 2003-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
class PQXX_LIBEXPORT binarystring
{
public:
  typedef unsigned char char_type;
  typedef std::char_traits<char_type>::char_type value_type;
  typedef size_t size_type;
  typedef long difference_type;
  typedef const value_type &const_reference;
  typedef const value_type *const_pointer;
  typedef const_pointer const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

public:
  binarystring(const binarystring &);

  /// Read and unescape bytea field
  /** The field will be zero-terminated, even if the original bytea field isn't.
   * @param F the field to read; must be a bytea field
   */
  explicit binarystring(const field &);					//[t62]

  /// Copy binary data from std::string.
  explicit binarystring(const std::string &);

  /// Copy binary data of given length straight out of memory.
  binarystring(const void *, size_t);

  ~binarystring() { delete &m_buf; }

  /// Size of converted string in bytes
  size_type size() const PQXX_NOEXCEPT { return m_size; }		//[t62]
  /// Size of converted string in bytes
  size_type length() const PQXX_NOEXCEPT { return size(); }		//[t62]
  bool empty() const PQXX_NOEXCEPT { return size()==0; }		//[t62]

  const_iterator begin() const PQXX_NOEXCEPT { return data(); }		//[t62]
  const_iterator cbegin() const PQXX_NOEXCEPT { return begin(); }
  const_iterator end() const PQXX_NOEXCEPT { return data()+m_size; }	//[t62]
  const_iterator cend() const PQXX_NOEXCEPT { return end(); }

  const_reference front() const PQXX_NOEXCEPT { return *begin(); }	//[t62]
  const_reference back() const PQXX_NOEXCEPT				//[t62]
	{ return *(data()+m_size-1); }

  const_reverse_iterator rbegin() const					//[t62]
	{ return const_reverse_iterator(end()); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  const_reverse_iterator rend() const					//[t62]
	{ return const_reverse_iterator(begin()); }
  const_reverse_iterator crend() const { return rend(); }

  /// Unescaped field contents
  const value_type *data() const PQXX_NOEXCEPT {return m_buf.get();}	//[t62]

  const_reference operator[](size_type i) const PQXX_NOEXCEPT		//[t62]
	{ return data()[i]; }

  PQXX_PURE bool operator==(const binarystring &) const PQXX_NOEXCEPT;	//[t62]
  bool operator!=(const binarystring &rhs) const PQXX_NOEXCEPT		//[t62]
	{ return !operator==(rhs); }

  binarystring &operator=(const binarystring &);

  /// Index contained string, checking for valid index
  const_reference at(size_type) const;					//[t62]

  /// Swap contents with other binarystring
  void swap(binarystring &);						//[t62]

  /// Raw character buffer (no terminating zero is added)
  /** @warning No terminating zero is added!  If the binary data did not end in
   * a null character, you will not find one here.
   */
  const char *get() const PQXX_NOEXCEPT					//[t62]
			{ return reinterpret_cast<const char *>(m_buf.get()); }

  /// Read as regular C++ string (may include null characters)
  /** @warning libpqxx releases before 3.1 stored the string and returned a
   * reference to it.  This is no longer the case!  It now creates and returns
   * a new string object.  Avoid repeated use of this function; retrieve your
   * string once and keep it in a local variable.  Also, do not expect to be
   * able to compare the string's address to that of an earlier invocation.
   */
  std::string str() const;						//[t62]

private:
  typedef internal::PQAlloc<
	value_type,
	pqxx::internal::freemallocmem_templated<unsigned char> >
    smart_pointer_type;

  /* Using a reference to a smart pointer.  It's wasteful, but it hides the
   * different implementations of PQAlloc: the compiler needs to know PQAlloc's
   * memory layout in order to include an instance inside binarystring.
   * Once shared_ptr is widespread enough, we can just have a shared_ptr and
   * forget about PQAlloc altogether.
   */
  smart_pointer_type &m_buf;
  size_type m_size;
};
}

#include "pqxx/compiler-internal-post.hxx"

#endif
