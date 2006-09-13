/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of libpqxx STL-style cursor classes.
 *   These classes wrap SQL cursors in STL-like interfaces
 *
 * Copyright (c) 2004-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cstdlib>

#include "pqxx/cursor"
#include "pqxx/result"
#include "pqxx/transaction"

using namespace PGSTD;


namespace
{
/// Compute actual displacement based on requested and reported displacements
pqxx::cursor_base::difference_type adjust(
    pqxx::cursor_base::difference_type d,
    pqxx::cursor_base::difference_type r)
{
  const pqxx::cursor_base::difference_type hoped = labs(d);
  pqxx::cursor_base::difference_type actual = r;
  if (hoped < 0 || r < hoped) ++actual;
  return (d < 0) ? -actual : actual;
}
}



pqxx::cursor_base::cursor_base(transaction_base *context,
    const PGSTD::string &Name,
    bool embellish_name) :
  m_context(context),
  m_done(false),
  m_name(embellish_name ? context->conn().adorn_name(Name) : Name),
  m_adopted(false),
  m_ownership(loose),
  m_lastfetch(),
  m_lastmove()
{
}


string pqxx::cursor_base::stridestring(difference_type n)
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


namespace
{
/// Is this character a "useless trailing character" in a query?
/** A character is "useless" at the end of a query if it is either whitespace or
 * a semicolon.
 */
inline bool useless_trail(char c)
{
  return isspace(c) || c==';';
}

}


void pqxx::cursor_base::declare(const PGSTD::string &query,
    accesspolicy ap,
    updatepolicy up,
    ownershippolicy op,
    bool hold)
{
  stringstream cq, qn;

  /* Strip trailing semicolons (and whitespace, as side effect) off query.  The
   * whitespace is stripped because it might otherwise mask a semicolon.  After
   * this, the remaining useful query will be the sequence defined by
   * query.begin() and last, i.e. last may be equal to query.end() or point to
   * the first useless trailing character.
   */
  string::const_iterator last = query.end();
  for (--last; last!=query.begin() && useless_trail(*last); --last);
  if (last==query.begin() && useless_trail(*last))
    throw invalid_argument("Cursor created on empty query");
  ++last;

  cq << "DECLARE \"" << name() << "\" ";

  if (m_context->conn().supports(connection_base::cap_cursor_scroll))
  {
    if (ap == forward_only) cq << "NO ";
    cq << "SCROLL ";
  }

  cq << "CURSOR ";

  if (hold)
  {
    if (!m_context->conn().supports(connection_base::cap_cursor_with_hold))
      throw runtime_error("Cursor " + name() + " "
	  "created for use outside of its originating transaction, "
	  "but this backend version does not support that.");
    cq << "WITH HOLD ";
  }

  cq << "FOR " << string(query.begin(),last) << ' ';

  if (up != update) cq << "FOR READ ONLY ";
  else if (!m_context->conn().supports(connection_base::cap_cursor_update))
    throw runtime_error("Cursor " + name() + " "
	"created as updatable, "
	"but this backend version does not support that.");
  else cq << "FOR UPDATE ";

  qn << "[DECLARE " << name() << ']';
  m_context->exec(cq, qn.str());

  // If we're creating a WITH HOLD cursor, noone is going to destroy it until
  // after this transaction.  That means the connection cannot be deactivated
  // without losing the cursor.
  m_ownership = op;
  if (op==loose) m_context->m_reactivation_avoidance.add(1);
}


void pqxx::cursor_base::adopt(ownershippolicy op)
{
  // If we take responsibility for destroying the cursor, that's one less reason
  // not to allow the connection to be deactivated and reactivated.
  if (op==owned) m_context->m_reactivation_avoidance.add(-1);
  m_adopted = true;
  m_ownership = op;
}


void pqxx::cursor_base::close() throw ()
{
  if (m_ownership==owned)
  {
    try
    {
      m_context->exec("CLOSE \"" + name() + "\"");
    }
    catch (const exception &)
    {
    }

    if (m_adopted) m_context->m_reactivation_avoidance.add(-1);
    m_ownership = loose;
  }
}


pqxx::result pqxx::cursor_base::fetch(difference_type n)
{
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


pqxx::result pqxx::cursor_base::fetch(difference_type n, difference_type &d)
{
  result r(cursor_base::fetch(n));
  d = adjust(n, r.size());
  return r;
}


template<> void
pqxx::cursor_base::check_displacement<pqxx::cursor_base::forward_only>(
    difference_type d) const
{
  if (d < 0)
    throw logic_error("Attempt to move cursor " + name() + " "
	"backwards (this cursor is only allowed to move forwards)");
}


pqxx::cursor_base::difference_type pqxx::cursor_base::move(difference_type n)
{
  if (!n) return 0;

  const string mq(
      (n == m_lastmove.dist) ?
      m_lastmove.query :
      "MOVE " + stridestring(n) + " IN \"" + name() + "\"");

  // Set m_done on exception (but no need for try/catch)
  m_done = true;
  const result r(m_context->exec(mq));

  // Starting with the libpq in PostgreSQL 7.4, PQcmdTuples() (which we call
  // indirectly here) also returns the number of rows skipped by a MOVE
  difference_type d = r.affected_rows();

  // We may not have PQcmdTuples(), or this may be a libpq version that doesn't
  // implement it for MOVE yet.  We'll also get zero if we decide to use a
  // prepared statement for the MOVE.
  if (!d)
  {
    static const string StdResponse("MOVE ");
    if (strncmp(r.CmdStatus(), StdResponse.c_str(), StdResponse.size()) != 0)
      throw internal_error("cursor MOVE returned "
	  "'" + string(r.CmdStatus()) + "' "
	  "(expected '" + StdResponse + "')");

    from_string(r.CmdStatus()+StdResponse.size(), d);
  }
  m_done = (d != n);
  return d;
}


pqxx::cursor_base::difference_type
pqxx::cursor_base::move(difference_type n, difference_type &d)
{
  const difference_type got = cursor_base::move(n);
  d = adjust(n, got);
  return got;
}


pqxx::icursorstream::icursorstream(pqxx::transaction_base &context,
    const PGSTD::string &query,
    const PGSTD::string &basename,
    difference_type Stride) :
  super(&context, query, basename),
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
  super(&Context, Name.c_str()),
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

pqxx::result pqxx::icursorstream::fetchblock()
{
  const result r(fetch(m_stride));
  m_realpos += r.size();
  return r;
}


pqxx::icursorstream &pqxx::icursorstream::ignore(PGSTD::streamsize n)
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
    const result r = fetchblock();
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


void pqxx::icursor_iterator::fill(const result &r)
{
  m_here = r;
}


