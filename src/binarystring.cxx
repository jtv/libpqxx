/*-------------------------------------------------------------------------
 *
 *   FILE
 *	binarystring.cxx
 *
 *   DESCRIPTION
 *      implementation of bytea (binary string) conversions
 *
 * Copyright (c) 2003-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

#include "libpq-fe.h"

#include "pqxx/binarystring"


using namespace pqxx::internal;

namespace
{
typedef unsigned char unsigned_char;
typedef std::pair<unsigned char *, size_t> buffer;


buffer to_buffer(const void *data, size_t len)
{
  void *const output(malloc(len + 1));
  if (!output) throw std::bad_alloc();
  static_cast<char *>(output)[len] = '\0';
  memcpy(static_cast<char *>(output), data, len);
  return buffer(static_cast<unsigned char *>(output), len);
}


buffer to_buffer(const std::string &source)
{
  return to_buffer(source.c_str(), source.size());
}



buffer unescape(const unsigned char escaped[])
{
#ifdef _WIN32
  /* On Windows only, the return value from PQunescapeBytea() must be freed
   * using PQfreemem.  Copy to a buffer allocated by libpqxx, so that the
   * binarystring's buffer can be freed uniformly,
   */
  size_t unescaped_len = 0;
  PQAlloc<unsigned char> A(
	PQunescapeBytea(const_cast<unsigned char *>(escaped), &unescaped_len));
  void *data = A.get();
  if (!data) throw std::bad_alloc();
  return to_buffer(data, unescaped_len);
#else
  /* On non-Windows platforms, it's okay to free libpq-allocated memory using
   * free().  No extra copy needed.
   */
  buffer unescaped;
  unescaped.first = PQunescapeBytea(
	const_cast<unsigned char *>(escaped), &unescaped.second);
  if (!unescaped.first) throw std::bad_alloc();
  return unescaped;
#endif
}

} // namespace


pqxx::binarystring::binarystring(const binarystring &rhs) :
  m_buf(*new smart_pointer_type(rhs.m_buf)),
  m_size(rhs.m_size)
{
}


pqxx::binarystring::binarystring(const field &F) :
  m_buf(*new smart_pointer_type),
  m_size(0)
{
  buffer unescaped(unescape(reinterpret_cast<const_pointer>(F.c_str())));
  m_buf = smart_pointer_type(unescaped.first);
  m_size = unescaped.second;
}


pqxx::binarystring::binarystring(const std::string &s) :
  m_buf(*new smart_pointer_type),
  m_size(s.size())
{
  m_buf = smart_pointer_type(to_buffer(s).first);
}


pqxx::binarystring::binarystring(const void *binary_data, size_t len) :
  m_buf(*new smart_pointer_type),
  m_size(len)
{
  m_buf = smart_pointer_type(to_buffer(binary_data, len).first);
}


bool pqxx::binarystring::operator==(const binarystring &rhs) const PQXX_NOEXCEPT
{
  if (rhs.size() != size()) return false;
  for (size_type i=0; i<size(); ++i) if (rhs[i] != data()[i]) return false;
  return true;
}


pqxx::binarystring &pqxx::binarystring::operator=(const binarystring &rhs)
{
  m_buf = rhs.m_buf;
  m_size = rhs.m_size;
  return *this;
}


pqxx::binarystring::const_reference pqxx::binarystring::at(size_type n) const
{
  if (n >= m_size)
  {
    if (!m_size)
      throw std::out_of_range("Accessing empty binarystring");
    throw std::out_of_range("binarystring index out of range: " +
	to_string(n) + " (should be below " + to_string(m_size) + ")");
  }
  return data()[n];
}


void pqxx::binarystring::swap(binarystring &rhs)
{
  // PQAlloc<>::swap() does not throw.
  m_buf.swap(rhs.m_buf);

  // This part very obviously can't go wrong, so do it last
  const size_type s(m_size);
  m_size = rhs.m_size;
  rhs.m_size = s;
}


std::string pqxx::binarystring::str() const
{
  return std::string(get(), m_size);
}
