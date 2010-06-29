/*-------------------------------------------------------------------------
 *
 *   FILE
 *	binarystring.cxx
 *
 *   DESCRIPTION
 *      implementation of bytea (binary string) conversions
 *
 * Copyright (c) 2003-2010, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
}

pqxx::binarystring::binarystring(const result::field &F) :
  super(),
  m_size(0)
{
  const unsigned char *const b(reinterpret_cast<const_iterator>(F.c_str()));

#ifdef PQXX_HAVE_PQUNESCAPEBYTEA
  unsigned char *const p = const_cast<unsigned char *>(b);

  size_t sz = 0;
  super::operator=(super(PQunescapeBytea(p, &sz)));
  if (!get()) throw bad_alloc();
  m_size = sz;

#else
  string s;
  const string::size_type len = F.size();
  s.reserve(len);

  if (len >= 2 && b[0] == '\\' && b[1] == 'x')
  {
    // Hex escape format (9.0+): \x3a20
    bool in_pair = false;
    int last_nibble = 0;
    for (result::field::size_type i=2; i<len; ++i)
    {
      const unsigned char c = b[i];
      if (isspace(c))
      {
        if (in_pair) throw out_of_range("Escaped binary data is malformed.");
      }
      else if (!isxdigit(c))
      {
	  throw out_of_range("Escaped binary data is malformed.");
      }
      else
      {
        const int nibble = (isdigit(c) ? DV(c) : (10 + tolower(c) - 'a'));
        if (in_pair) s += char((last_nibble<<4) | nibble);
        else last_nibble = nibble;
        in_pair = !in_pair;
      }
    }
    if (in_pair) throw out_of_range("Escaped binary data appears truncated.");
  }
  else
  {
    // Octal escape format (pre-9.0): a\123b
    for (result::field::size_type i=0; i<len; ++i)
    {
      unsigned char c = b[i];
      if (c == '\\')
      {
	c = b[++i];
	if (isdigit(c) && isdigit(b[i+1]) && isdigit(b[i+2]))
	{
	  c = unsigned_char((DV(c)<<6) | (DV(b[i+1])<<3) | DV(b[i+2]));
	  i += 2;
	}
      }
      s += char(c);
    }
  }

  m_size = s.size();
  void *buf = malloc(m_size+1);
  if (!buf) throw bad_alloc();
  super::operator=(super(static_cast<unsigned char *>(buf)));
  memcpy(static_cast<char *>(buf), s.c_str(), m_size);

#endif
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


