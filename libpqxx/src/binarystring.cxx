/*-------------------------------------------------------------------------
 *
 *   FILE
 *	binarystring.cxx
 *
 *   DESCRIPTION
 *      implementation of bytea (binary string) conversions
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <stdexcept>

#include "pqxx/binarystring"

using namespace PGSTD;

namespace
{
// Convert textual digit to value
inline char DV(unsigned char d) { return d - '0'; }
}

pqxx::binarystring::binarystring(const result::field &F) :
  super(),
  m_size(0)
{
  unsigned char *p = const_cast<unsigned char *>(
      reinterpret_cast<const_iterator>(F.c_str()));

#ifdef PQXX_HAVE_PQUNESCAPEBYTEA

  size_t sz = 0;
  super::operator=(PQunescapeBytea(p, &sz));
  // TODO: More useful error message--report cause of the problem!
  if (!c_ptr()) throw runtime_error("Unable to read bytea field");
  m_size = sz;

#else

  string result;
  for (int i=0; i<F.size(); ++i)
  {
    unsigned char c = p[i];
    if (c == '\\')
    {
      c = p[++i];
      if (isdigit(c) && isdigit(p[i+1]) && isdigit(p[i+2]))
      {
	c = (DV(p[c])<<9) | (DV(p[i+1])<<3) | DV(p[i+2]);
	i += 2;
      }
    }
    result += char(c);
  }

  m_size = result.size();
  char *buf = malloc(m_size+1);
  if (!buf)
    throw bad_alloc();
  super::operator=(buf);
  strcpy(buf, result.c_str());

#endif
}


pqxx::binarystring::const_reference pqxx::binarystring::at(size_type n) const
{
  if (n >= m_size)
  {
    if (!m_size)
      throw out_of_range("Accessing empty binarystring");
    throw out_of_range("binarystring index out of range: " +
	ToString(n) + " (should be below " + ToString(m_size) + ")");
  }
  return data()[n];
}


const string &pqxx::binarystring::str() const
{
  if (m_str.empty() && m_size) m_str = string(c_ptr(), m_size);
  return m_str;
}


string pqxx::escape_binary(const unsigned char bin[], size_t len)
{
#ifdef PQXX_HAVE_PQUNESCAPEBYTEA
  size_t escapedlen = 0;
  unsigned char *p = const_cast<unsigned char *>(bin);
  PQAlloc<unsigned char> A(PQescapeBytea(p, len, &escapedlen));
  const char *cstr = reinterpret_cast<const char *>(A.c_ptr());
  // TODO: Find out the nature of the problem, if any!
  if (!cstr) throw runtime_error("Could not escape binary string!");
  return string(cstr, escapedlen-1);
#else
  string result;
  for (int i=0; i<len; ++i)
  {
    if (bin[i] & 0x80)
    {
      char buf[8];
      sprintf(buf, "\\\\%03o", unsigned(bin[i]));
      result += buf;
    }
    else switch (bin[i])
    {
    case 0:
      result += "\\\\000";
      break;

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


