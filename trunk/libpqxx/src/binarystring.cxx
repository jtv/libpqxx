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
#include <stdexcept>

#include "pqxx/binarystring"

using namespace std;

pqxx::binarystring::binarystring(const result::field &F) :
  super(),
  m_size(0)
{
  size_t sz = 0;
  unsigned char *p = const_cast<unsigned char *>(
      reinterpret_cast<const_iterator>(F.c_str()));
  super::operator=(PQunescapeBytea(p, &sz));
  // TODO: More useful error message--report cause of the problem!
  if (!c_ptr()) throw runtime_error("Unable to read bytea field");
  m_size = sz;
}


pqxx::binarystring::const_reference pqxx::binarystring::at(size_type n) const
{
  if ((n < 0) || (n >= m_size))
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
  size_t escapedlen = 0;
  unsigned char *p = const_cast<unsigned char *>(bin);
  PQAlloc<unsigned char> A(PQescapeBytea(p, len, &escapedlen));
  const char *cstr = reinterpret_cast<const char *>(A.c_ptr());
  // TODO: Find out the nature of the problem, if any!
  if (!cstr) throw runtime_error("Could not escape binary string!");
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


