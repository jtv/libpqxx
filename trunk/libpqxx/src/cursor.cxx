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


unsigned char pqxx::cursor_base::s_dummy;


pqxx::icursorstream::icursorstream(pqxx::transaction_base &context,
    const string &query,
    const string &basename,
    difference_type stride) :
  cursor_base(&context, basename),
  m_stride(stride)
{
  set_stride(stride);
  declare(query);
}

void pqxx::icursorstream::set_stride(difference_type n)
{
  if (n < 1)
    throw invalid_argument("Attempt to set cursor stride to " + to_string(n));
  m_stride = n;
}

void pqxx::icursorstream::declare(const string &query)
{
  // TODO: Add NO SCROLL if backend supports it (7.4 or better)
  stringstream cq, qn;
  cq << "DECLARE \"" << name() << "\" CURSOR FOR " << query << " FOR READ ONLY";
  qn << "[DECLARE " << name() << ']';
  m_context->exec(cq, qn.str());
}


pqxx::result pqxx::icursorstream::fetch()
{
  result r(m_context->exec("FETCH "+to_string(m_stride)+" IN \""+name()+"\""));
  if (r.empty()) m_done = true;
  return r;
}


pqxx::icursorstream &pqxx::icursorstream::ignore(streamsize n)
{
  m_context->exec("MOVE " + to_string(n) + " IN \"" + name() + "\"");
  return *this;
}


