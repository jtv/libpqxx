/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of libpqxx STL-style cursor classes.
 *   These classes wrap SQL cursors in STL-like interfaces
 *
 * Copyright (c) 2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <cassert>
#include <cstdlib>

#include "pqxx/cursor"
#include "pqxx/result"
#include "pqxx/transaction"

using namespace PGSTD;


pqxx::cursor_base::cursor_base(transaction_base *context,
    const string &cname,
    bool embellish_name) :
  m_context(context),
  m_done(false),
  m_name(cname)
{
  if (embellish_name)
  {
    m_name += '_';
    m_name += to_string(get_unique_cursor_num());
  }
}


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
    difference_type Stride) :
  cursor_base(&context, basename),
  m_stride(Stride),
  m_realpos(0),
  m_reqpos(0),
  m_iterators(0)
{
  set_stride(Stride);
  declare(query);
}


pqxx::icursorstream::icursorstream(transaction_base &Context,
    const result::field &Name,
    difference_type Stride) :
  cursor_base(&Context, Name.c_str(), false),
  m_stride(Stride),
  m_realpos(0),
  m_reqpos(0),
  m_iterators(0)
{
  set_stride(Stride);
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
  m_realpos += r.size();
  return r;
}


pqxx::icursorstream &pqxx::icursorstream::ignore(streamsize n)
{
  m_context->exec("MOVE " + to_string(n) + " IN \"" + name() + "\"");
  // TODO: Try to get actual number of moved tuples
  m_realpos += n;
  return *this;
}


pqxx::icursorstream::size_type pqxx::icursorstream::forward(size_type n)
{
  m_reqpos += n*m_stride;
  return m_reqpos;
}


void pqxx::icursorstream::insert_iterator(icursor_iterator *i) throw ()
{
  assert(i);
  assert(i->m_stream==this);
  assert(!i->m_next);
  assert(!i->m_prev);
#ifndef NDEBUG
  for (icursor_iterator *s=m_iterators;s;s=s->m_next)assert(s!=i);
#endif
  i->m_next = m_iterators;
  if (m_iterators) m_iterators->m_prev = i;
  m_iterators = i;
}


void pqxx::icursorstream::remove_iterator(icursor_iterator *i) const throw ()
{
  assert(i);
  assert(i->m_stream == this);
  assert(m_iterators);
  if (i == m_iterators)
  {
    assert(!i->m_prev);
    m_iterators = i->m_next;
    if (m_iterators)
    {
      assert(m_iterators->m_prev == i);
      m_iterators->m_prev = 0;
    }
  }
  else
  {
    assert(i->m_prev);
    assert(i->m_prev->m_next == i);
    i->m_prev->m_next = i->m_next;
    if (i->m_next) i->m_next->m_prev = i->m_prev;
  }
  i->m_prev = 0;
  i->m_next = 0;
}


void pqxx::icursorstream::service_iterators(size_type topos)
{
  assert(topos <= m_reqpos);
  if (topos < m_realpos) return;

  typedef multimap<size_type,icursor_iterator*> todolist;
  todolist todo;
  for (icursor_iterator *i = m_iterators; i; i = i->m_next)
    if (i->m_pos >= m_realpos && i->m_pos <= topos)
      todo.insert(make_pair(i->m_pos, i));
  for (todolist::const_iterator i = todo.begin(); i != todo.end(); )
  {
    const size_type readpos = i->first;
    if (readpos > m_realpos) ignore(readpos - m_realpos);
    assert(m_realpos == i->first);
    const result r = fetch();
    for ( ; i != todo.end() && i->first == readpos; ++i)
      i->second->fill(r);
  }
}


pqxx::icursor_iterator::icursor_iterator() throw () :
  m_stream(0),
  m_here(),
  m_pos(0),
  m_prev(0),
  m_next(0)
{
}

pqxx::icursor_iterator::icursor_iterator(istream_type &s) throw () :
  m_stream(&s),
  m_here(),
  m_pos(s.forward(0)),
  m_prev(0),
  m_next(0)
{
  s.insert_iterator(this);
}

pqxx::icursor_iterator::icursor_iterator(const icursor_iterator &rhs) throw () :
  m_stream(rhs.m_stream),
  m_here(rhs.m_here),
  m_pos(rhs.m_pos),
  m_prev(0),
  m_next(0)
{
  if (m_stream) m_stream->insert_iterator(this);
}


pqxx::icursor_iterator::~icursor_iterator() throw ()
{
  if (m_stream) m_stream->remove_iterator(this);
}


pqxx::icursor_iterator pqxx::icursor_iterator::operator++(int)
{
  icursor_iterator old(*this);
  m_pos = m_stream->forward();
  m_here.clear();
  return old;
}


pqxx::icursor_iterator &pqxx::icursor_iterator::operator++()
{
  m_pos = m_stream->forward();
  m_here.clear();
  return *this;
}


pqxx::icursor_iterator &pqxx::icursor_iterator::operator+=(difference_type n)
{
  if (n <= 0)
  {
    if (!n) return *this;
    throw invalid_argument("Advancing icursor_iterator by negative offset");
  }
  m_pos = m_stream->forward(n);
  m_here.clear();
  return *this;
}


pqxx::icursor_iterator &
pqxx::icursor_iterator::operator=(const icursor_iterator &rhs) throw ()
{
  if (rhs.m_stream == m_stream)
  {
    m_here = rhs.m_here;
    m_pos = rhs.m_pos;
  }
  else
  {
    if (m_stream) m_stream->remove_iterator(this);
    m_here = rhs.m_here;
    m_pos = rhs.m_pos;
    m_stream = rhs.m_stream;
    if (m_stream) m_stream->insert_iterator(this);
  }
  return *this;
}


bool pqxx::icursor_iterator::operator==(const icursor_iterator &rhs) const
{
  if (m_stream == rhs.m_stream) return pos() == rhs.pos();
  if (m_stream && rhs.m_stream) return false;
  refresh();
  rhs.refresh();
  return m_here.empty() && rhs.m_here.empty();
}


bool pqxx::icursor_iterator::operator<(const icursor_iterator &rhs) const
{
  if (m_stream == rhs.m_stream) return pos() < rhs.pos();
  refresh();
  rhs.refresh();
  return !m_here.empty();
}


void pqxx::icursor_iterator::refresh() const
{
  if (m_stream) m_stream->service_iterators(pos());
}


void pqxx::icursor_iterator::fill(const result &r) const
{
  m_here = r;
}


