/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of libpqxx STL-style cursor classes.
 *   These classes wrap SQL cursors in STL-like interfaces
 *
 * Copyright (c) 2004-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


string pqxx::cursor_base::stridestring(pqxx::cursor_base::difference_type n)
{
  /* Some special-casing for ALL and BACKWARD ALL here.  We used to use numeric
   * "infinities" for difference_type for this (the highest and lowest possible
   * values for "long"), but for PostgreSQL 8.0 at least, the backend appears to
   * expect a 32-bit number and fails to parse large 64-bit numbers.
   * We could change the typedef to match this behaviour, but that would break
   * if/when Postgres is changed to accept 64-bit displacements.
   */
  static const string All("ALL"), BackAll("BACKWARD ALL");
  if (n == all()) return All;
  else if (n == backward_all()) return BackAll;
  return to_string(n);
}


pqxx::basic_cursor::basic_cursor(transaction_base *t,
    const string &query,
    const string &cname) :
  cursor_base(t, cname, true),
  m_lastfetch(),
  m_lastmove()
{
  // TODO: Add NO SCROLL if backend supports it (7.4 or better)
  stringstream cq, qn;
  cq << "DECLARE \"" << name() << "\" CURSOR FOR " << query << " FOR READ ONLY";
  qn << "[DECLARE " << name() << ']';
  m_context->exec(cq, qn.str());
}


pqxx::basic_cursor::basic_cursor(transaction_base *t,
    const string &cname) :
  cursor_base(t, cname, false),
  m_lastfetch(),
  m_lastmove()
{
}


pqxx::result pqxx::basic_cursor::fetch(difference_type n)
{
  // TODO: Use prepared statement for fetches/moves?

  result r;
  if (n) 
  {
    // We cache the last-executed fetch query.  If the current fetch uses the
    // same distance, we just re-use the cached string instead of composing a
    // new one.
    const string fq(
	(n == m_lastfetch.dist) ?
         m_lastfetch.query :
         "FETCH " + stridestring(n) + " IN \"" + name() + "\"");

    // Set m_done on exception (but no need for try/catch)
    m_done = true;
    r = m_context->exec(fq);
    if (!r.empty()) m_done = false;
  }
  return r;
}


pqxx::cursor_base::difference_type pqxx::basic_cursor::move(difference_type n)
{
  if (!n) return 0;

  const string mq(
      (n == m_lastmove.dist) ?
      m_lastmove.query :
      "MOVE " + stridestring(n) + " IN \"" + name() + "\"");

  // Set m_done on exception (but no need for try/catch)
  m_done = true;
  const result r(m_context->exec(mq));

  static const string StdResponse("MOVE ");
  if (strncmp(r.CmdStatus(), StdResponse.c_str(), StdResponse.size()) != 0)
    throw logic_error("libpqxx internal error: "
	"cursor MOVE returned '" + string(r.CmdStatus()) + "' "
	"(expected '" + StdResponse + "')");

  difference_type d;
  from_string(r.CmdStatus()+StdResponse.size(), d);
  m_done = (d != n);
  return d;
}


pqxx::icursorstream::icursorstream(pqxx::transaction_base &context,
    const string &query,
    const string &basename,
    difference_type Stride) :
  basic_cursor(&context, query, basename),
  m_stride(Stride),
  m_realpos(0),
  m_reqpos(0),
  m_iterators(0)
{
  set_stride(Stride);
}


pqxx::icursorstream::icursorstream(transaction_base &Context,
    const result::field &Name,
    difference_type Stride) :
  basic_cursor(&Context, Name.c_str()),
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

pqxx::result pqxx::icursorstream::fetch()
{
  const result r(basic_cursor::fetch(m_stride));
  m_realpos += r.size();
  return r;
}


pqxx::icursorstream &pqxx::icursorstream::ignore(streamsize n)
{
  m_realpos += move(n);
  return *this;
}


pqxx::icursorstream::size_type pqxx::icursorstream::forward(size_type n)
{
  m_reqpos += n*m_stride;
  return m_reqpos;
}


void pqxx::icursorstream::insert_iterator(icursor_iterator *i) throw ()
{
  i->m_next = m_iterators;
  if (m_iterators) m_iterators->m_prev = i;
  m_iterators = i;
}


void pqxx::icursorstream::remove_iterator(icursor_iterator *i) const throw ()
{
  if (i == m_iterators)
  {
    m_iterators = i->m_next;
    if (m_iterators) m_iterators->m_prev = 0;
  }
  else
  {
    i->m_prev->m_next = i->m_next;
    if (i->m_next) i->m_next->m_prev = i->m_prev;
  }
  i->m_prev = 0;
  i->m_next = 0;
}


void pqxx::icursorstream::service_iterators(size_type topos)
{
  if (topos < m_realpos) return;

  typedef multimap<size_type,icursor_iterator*> todolist;
  todolist todo;
  for (icursor_iterator *i = m_iterators; i; i = i->m_next)
    if (i->m_pos >= m_realpos && i->m_pos <= topos)
      todo.insert(todolist::value_type(i->m_pos, i));
  const todolist::const_iterator todo_end(todo.end());
  for (todolist::const_iterator i = todo.begin(); i != todo_end; )
  {
    const size_type readpos = i->first;
    if (readpos > m_realpos) ignore(readpos - m_realpos);
    const result r = fetch();
    for ( ; i != todo_end && i->first == readpos; ++i)
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


