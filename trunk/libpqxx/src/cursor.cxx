/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::Cursor class.
 *   pqxx::Cursor represents a database cursor.
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <cstdlib>

#include "pqxx/cursor"

#include "pqxx/result"
#include "pqxx/transaction"

using namespace PGSTD;


int pqxx::cursor_base::get_unique_cursor_num()
{
  if (!m_context) throw logic_error("libpqxx internal error: "
      "cursor in get_unique_cursor_num() has no transaction");
  return m_context->GetUniqueCursorNum();
}


pqxx::const_forward_cursor::const_forward_cursor() :
  cursor_base(),
  m_name(),
  m_stride(0),
  m_lastdata()
{
}


pqxx::const_forward_cursor::const_forward_cursor(const const_forward_cursor &x):
  cursor_base(x.m_context),
  m_name(x.m_name),
  m_stride(x.m_stride),
  m_lastdata(x.m_lastdata)
{
}


pqxx::const_forward_cursor::const_forward_cursor(transaction_base &context,
    const string &query,
    size_type stride) :
  cursor_base(&context),
  m_name(),
  m_stride(stride),
  m_lastdata()
{
  m_name = m_context->name() + "_cfc";
  declare(query);
  fetch();
}


pqxx::const_forward_cursor::const_forward_cursor(transaction_base &context,
    const string &query,
    const string &cname,
    size_type stride) :
  cursor_base(&context),
  m_name(cname),
  m_stride(stride),
  m_lastdata()
{
  declare(query);
  fetch();
}


pqxx::const_forward_cursor &pqxx::const_forward_cursor::operator++()
{
  fetch();
  return *this;
}


pqxx::const_forward_cursor pqxx::const_forward_cursor::operator++(int)
{
  fetch();
  return *this;
}


bool pqxx::const_forward_cursor::operator==(const const_forward_cursor &x) const
{
  // Result is only interesting for "at-end" comparisons
  return m_lastdata.empty() && x.m_lastdata.empty();
}


pqxx::const_forward_cursor &
pqxx::const_forward_cursor::operator=(const const_forward_cursor &x)
{
  if (&x != this)
  {
    // Copy to temporaries to improve exception robustness
    result lastdata_tmp(x.m_lastdata);
    string name_tmp(x.m_name);

    // Object's state change should be exception-free
    m_lastdata.swap(lastdata_tmp);
    m_name.swap(name_tmp);
    m_stride = x.m_stride;
    m_context = x.m_context;
  }
  return *this;
}


void pqxx::const_forward_cursor::declare(const string &query)
{
  m_name += "_";
  m_name += to_string(get_unique_cursor_num());

  m_context->exec("DECLARE \""+m_name+"\" NO SCROLL FOR READ ONLY FOR "+query,
	"[DECLARE "+m_name+"]");
}


void pqxx::const_forward_cursor::fetch()
{
  m_lastdata =
  	m_context->exec("FETCH "+to_string(m_stride)+" IN \""+m_name+"\"");
}


