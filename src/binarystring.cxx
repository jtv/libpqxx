/*-------------------------------------------------------------------------
 *
 *   FILE
 *	binarystring.cxx
 *
 *   DESCRIPTION
 *      implementation of bytea (binary string) conversions
 *
 * Copyright (c) 2003-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


using namespace PGSTD;
using namespace pqxx::internal;

namespace
{
typedef unsigned char unsigned_char;

#ifndef PQXX_HAVE_PQUNESCAPEBYTEA_9
// Convert textual digit to value
inline unsigned char DV(unsigned char d)
{
  return unsigned_char(digit_to_number(char(d)));
}
#endif


typedef pair<const unsigned char *, size_t> buffer;


buffer to_buffer(const void *data, size_t len)
{
  void *const output(malloc(len + 1));
  if (!output) throw bad_alloc();
  static_cast<char *>(output)[len] = '\0';
  memcpy(static_cast<char *>(output), data, len);
  return buffer(static_cast<unsigned char *>(output), len);
}


buffer to_buffer(const string &source)
{
  return to_buffer(source.c_str(), source.size());
}



buffer builtin_unescape(const unsigned char escaped[], size_t)
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
  if (!data) throw bad_alloc();
  return to_buffer(data, unescaped_len);
#else
  /* On non-Windows platforms, it's okay to free libpq-allocated memory using
   * free().  No extra copy needed.
   */
  buffer unescaped;
  unescaped.first = PQunescapeBytea(
	const_cast<unsigned char *>(escaped), &unescaped.second);
  if (!unescaped.first) throw bad_alloc();
  return unescaped;
#endif
}


#ifndef PQXX_HAVE_PQUNESCAPEBYTEA_9
/// Is this in PostgreSQL 9.0 hex-escaped binary format?
bool is_hex(const unsigned char buf[], size_t len)
{
  return len >= 2 && buf[0] == '\\' && buf[1] == 'x';
}
#endif


#ifndef PQXX_HAVE_PQUNESCAPEBYTEA_9
/// Unescape PostgreSQL 9.0 hex-escaped binary format: "\x3a20"
string unescape_hex(const unsigned char buf[], size_t len)
{
  string bin;
  bin.reserve((len-2)/2);
  bool in_pair = false;
  int last_nibble = 0;

  for (size_t i=2; i<len; ++i)
  {
    const unsigned char c = buf[i];
    if (isspace(c))
    {
      if (in_pair) throw out_of_range("Escaped binary data is malformed.");
    }
    else if (!isxdigit(c))
    {
      throw out_of_range("Escaped binary data contains invalid characters.");
    }
    else
    {
      const int nibble = (isdigit(c) ? DV(c) : (10 + tolower(c) - 'a'));
      if (in_pair) bin += char((last_nibble<<4) | nibble);
      else last_nibble = nibble;
      in_pair = !in_pair;
    }
  }
  if (in_pair) throw out_of_range("Escaped binary data appears truncated.");

  return bin;
}
#endif


buffer unescape(const unsigned char escaped[], size_t len)
{
#if defined(PQXX_HAVE_PQUNESCAPEBYTEA_9)
  return builtin_unescape(escaped, len);
#else
  // Supports octal format but not the newer hex format.
  if (is_hex(escaped, len)) return to_buffer(unescape_hex(escaped, len));
  else return builtin_unescape(escaped, len);
#endif
}

} // namespace


pqxx::binarystring::binarystring(const field &F) :
  super(),
  m_size(0)
{
  buffer unescaped(
	unescape(reinterpret_cast<const_pointer>(F.c_str()), F.size()));
  super::operator=(super(unescaped.first));
  m_size = unescaped.second;
}


pqxx::binarystring::binarystring(const string &s) :
  super(),
  m_size(s.size())
{
  super::operator=(super(to_buffer(s).first));
}


pqxx::binarystring::binarystring(const void *binary_data, size_t len) :
  super(),
  m_size(len)
{
  super::operator=(super(to_buffer(binary_data, len).first));
}


bool pqxx::binarystring::operator==(const binarystring &rhs) const throw ()
{
  if (rhs.size() != size()) return false;
  for (size_type i=0; i<size(); ++i) if (rhs[i] != data()[i]) return false;
  return true;
}


pqxx::binarystring::const_reference pqxx::binarystring::at(size_type n) const
{
  if (n >= m_size)
  {
    if (!m_size)
      throw out_of_range("Accessing empty binarystring");
    throw out_of_range("binarystring index out of range: " +
	to_string(n) + " (should be below " + to_string(m_size) + ")");
  }
  return data()[n];
}


void pqxx::binarystring::swap(binarystring &rhs)
{
  // PQAlloc<>::swap() is nothrow
  super::swap(rhs);

  // This part very obviously can't go wrong, so do it last
  const size_type s(m_size);
  m_size = rhs.m_size;
  rhs.m_size = s;
}


string pqxx::binarystring::str() const
{
  return string(get(), m_size);
}


string pqxx::escape_binary(const unsigned char bin[], size_t len)
{
  size_t escapedlen = 0;
  unsigned char *p = const_cast<unsigned char *>(bin);
  PQAlloc<unsigned char> A(PQescapeBytea(p, len, &escapedlen));
  const char *cstr = reinterpret_cast<const char *>(A.get());
  if (!cstr) throw bad_alloc();
  return string(cstr, escapedlen-1);
}

string pqxx::escape_binary(const unsigned char bin[])
{
  return escape_binary(bin, strlen(reinterpret_cast<const char *>(bin)));
}

string pqxx::escape_binary(const char bin[], size_t len)
{
  return escape_binary(reinterpret_cast<const unsigned char *>(bin), len);
}

string pqxx::escape_binary(const char bin[])
{
  return escape_binary(bin, strlen(bin));
}

string pqxx::escape_binary(const string &bin)
{
  return escape_binary(bin.c_str(), bin.size());
}


