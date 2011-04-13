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

// Convert textual digit to value
inline unsigned char DV(unsigned char d)
{
  return unsigned_char(digit_to_number(char(d)));
}


#if !defined(PQXX_HAVE_PQUNESCAPEBYTEA_9) && !defined(PQXX_HAVE_PQUNESCAPEBYTEA)
/// Unescape PostgreSQL pre-9.0 octal-escaped binary format: a\123b
string unescape_oct(const unsigned char buf[], size_t len)
{
  string bin;
  bin.reserve(len);

  for (size_t i=0; i<len; ++i)
  {
    unsigned char c = buf[i];
    if (c == '\\')
    {
      c = buf[++i];
      if (isdigit(c) && isdigit(buf[i+1]) && isdigit(buf[i+2]))
      {
	c = unsigned_char((DV(c)<<6) | (DV(buf[i+1])<<3) | DV(buf[i+2]));
	i += 2;
      }
    }
    bin += char(c);
  }

  return bin;
}
#endif


typedef pair<const unsigned char *, size_t> buffer;


#if defined(PQXX_HAVE_PQUNESCAPEBYTEA)
buffer builtin_unescape(const unsigned char escaped[], size_t)
{
  buffer unescaped;
  unescaped.first = PQunescapeBytea(
	const_cast<unsigned char *>(escaped), &unescaped.second);
  if (!unescaped.first) throw bad_alloc();
  return unescaped;
}
#endif


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


#ifndef PQXX_HAVE_PQUNESCAPEBYTEA_9
buffer to_buffer(const string &source)
{
  void *const output(malloc(source.size() + 1));
  if (!output) throw bad_alloc();
  memcpy(static_cast<char *>(output), source.c_str(), source.size());
  return buffer(static_cast<unsigned char *>(output), source.size());
}
#endif


buffer unescape(const unsigned char escaped[], size_t len)
{
#if defined(PQXX_HAVE_PQUNESCAPEBYTEA_9)
  return builtin_unescape(escaped, len);
#elif defined(PQXX_HAVE_PQUNESCAPEBYTEA)
  // Supports octal format but not the newer hex format.
  if (is_hex(escaped, len)) return to_buffer(unescape_hex(escaped, len));
  else return builtin_unescape(escaped, len);
#else
  return to_buffer(
	is_hex(escaped, len) ?
		unescape_hex(escaped, len) : unescape_oct(escaped, len));
#endif
}

} // namespace


pqxx::binarystring::binarystring(const result::field &F) :
  super(),
  m_size(0)
{
  buffer unescaped(
	unescape(reinterpret_cast<const_pointer>(F.c_str()), F.size()));
  super::operator=(super(unescaped.first));
  m_size = unescaped.second;
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
#ifdef PQXX_HAVE_PQESCAPEBYTEA
  size_t escapedlen = 0;
  unsigned char *p = const_cast<unsigned char *>(bin);
  PQAlloc<unsigned char> A(PQescapeBytea(p, len, &escapedlen));
  const char *cstr = reinterpret_cast<const char *>(A.get());
  if (!cstr) throw bad_alloc();
  return string(cstr, escapedlen-1);
#else
  /* Very basic workaround for missing PQescapeBytea() in antique versions of
   * libpq.  Clients that use BYTEA are much better off upgrading their libpq,
   * but this might just provide usable service in cases where that is not an
   * option.
   */
  string result;
  result.reserve(len);
  for (size_t i=0; i<len; ++i)
  {
    if (bin[i] >= 0x80 || bin[i] < 0x20)
    {
      char buf[8];
      sprintf(buf, "\\\\%03o", unsigned(bin[i]));
      result += buf;
    }
    else switch (bin[i])
    {
    case '\'':
      result += "\\'";
      break;

    case '\\':
      result += "\\\\\\\\";
      break;

    default:
      result += char(bin[i]);
    }
  }
  return result;
#endif
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


