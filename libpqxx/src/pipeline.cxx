/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pipeline.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::pipeline class
 *   Throughput-optimized query manager
 *
 * Copyright (c) 2003-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <algorithm>
#include <cassert> // DEBUG CODE

#include "pqxx/dbtransaction"
#include "pqxx/pipeline"

using namespace PGSTD;
using namespace pqxx;

pqxx::pipeline::pipeline(transaction_base &t, const string &PName) :
  internal::transactionfocus(t, PName, "pipeline"),
  m_queries(),
  m_issuedrange(0,0),
  m_retain(0),
  m_num_waiting(0),
  m_q_id(0),
  m_dummy_pending(false),
  m_error(qid_limit())
{
  invariant();
  register_me();
}


pqxx::pipeline::~pipeline() throw ()
{
  try { flush(); } catch (const exception &) {}
  unregister_me();
}


pipeline::query_id pqxx::pipeline::insert(const string &q)
{
  invariant();

  const query_id qid = generate_id();
  assert(qid > 0);
  assert(m_queries.find(qid)==m_queries.end());
  m_queries.insert(make_pair(qid,Query(q)));
  if (m_queries.lower_bound(m_issuedrange.first)->first == qid)
  {
    assert(!have_pending());
    m_issuedrange.first = m_issuedrange.second = qid;
  }
  m_num_waiting++;
  if (m_num_waiting > m_retain)
  {
    if (have_pending()) receive_if_available();
    if (!have_pending()) issue();
  }

  invariant();

  return qid;
}


void pqxx::pipeline::complete()
{
  invariant();

  if (have_pending()) receive(end_of_issued());
  if (m_num_waiting && (m_error == qid_limit()))
  {
    assert(!have_pending());
    issue(m_queries.end());
    assert(!m_num_waiting);
    assert(have_pending());
    assert(end_of_issued() == m_queries.end());
    receive(m_queries.end());
    assert((m_error!=qid_limit()) || !have_pending());
  }
  invariant();
  assert((m_num_waiting == 0) || (m_error != qid_limit()));
  assert(!m_dummy_pending);
}


void pqxx::pipeline::flush()
{
  invariant();

  if (m_queries.empty()) return;
  if (have_pending()) receive(end_of_issued());
  m_issuedrange.first = m_issuedrange.second = m_q_id + 1;
  m_num_waiting = 0;
  m_dummy_pending = false;
  m_queries.clear();

  invariant();
}


pair<pipeline::query_id, result> pqxx::pipeline::retrieve()
{
  if (m_queries.empty())
    throw logic_error("Attempt to retrieve result from empty pipeline");
  return retrieve(m_queries.begin());
}


int pqxx::pipeline::retain(int retain_max)
{
  invariant();

  if (retain_max < 0)
    throw range_error("Attempt to make pipeline retain " +
	to_string(retain_max) + " queries");

  const int oldvalue = m_retain;
  m_retain = retain_max;

  if (m_num_waiting >= m_retain) resume();

  invariant();

  return oldvalue;
}


void pqxx::pipeline::resume()
{
  invariant();

  if (have_pending()) receive_if_available();
  if (!have_pending() && m_num_waiting)
  {
    issue();
    receive_if_available();
  }

  invariant();
}


void pqxx::pipeline::invariant() const // DEBUG CODE
{
  assert(m_q_id >= 0);
  assert(m_q_id <= qid_limit());

  assert(m_retain >= 0);
  assert(m_num_waiting >= 0);

  assert(m_issuedrange.first <= m_issuedrange.second);
  assert(m_issuedrange.first >= 0);
  assert(m_issuedrange.second <= qid_limit());

  assert(have_pending() == (m_issuedrange.first < m_issuedrange.second));

  if (!m_queries.empty())
  {
    const query_id oldest = m_queries.begin()->first,
	           newest = m_queries.rbegin()->first;

    assert(oldest > 0);
    assert(oldest <= newest);
    assert(newest <= m_q_id);
    assert(m_issuedrange.first >= oldest);

    assert(size_t(m_queries.size()) <= 1+size_t(newest-oldest));
    assert(m_num_waiting >= 0);
    assert(size_t(m_num_waiting) <= size_t(m_queries.size()));
    if (have_pending())
    {
      const QueryMap::const_iterator pending_lower = oldest_issued(),
	                             pending_upper = end_of_issued();

      assert(pending_lower != m_queries.end());
      assert(pending_lower->first <= newest);
      assert(distance(pending_lower, pending_upper) > 0);

      assert(m_num_waiting == distance(pending_upper, m_queries.end()));
    }
  }
  else
  {
    assert(!have_pending());
    assert(!m_num_waiting);
  }
  if (m_dummy_pending) assert(have_pending());

  assert(m_error != 0);
}


pipeline::query_id pqxx::pipeline::generate_id()
{
  if (m_q_id == qid_limit())
    throw overflow_error("Too many queries went through pipeline");
  ++m_q_id;
  return m_q_id;
}


pipeline::QueryMap::const_iterator pqxx::pipeline::oldest_issued() const
{
  const QueryMap::const_iterator o = m_queries.find(m_issuedrange.first);
  assert(o != m_queries.end());
  return o;
}


pipeline::QueryMap::iterator pqxx::pipeline::oldest_issued()
{
  const QueryMap::iterator o = m_queries.find(m_issuedrange.first);
  if (o == m_queries.end())
    internal_error("libpqxx internal error: cannot find oldest pending query");
  return o;
}


pipeline::QueryMap::const_iterator pqxx::pipeline::end_of_issued() const
{
  QueryMap::const_iterator e = m_queries.lower_bound(m_issuedrange.second);
  assert((e == m_queries.end()) || (e->first >= m_issuedrange.second));
  return e;
}


pipeline::QueryMap::iterator pqxx::pipeline::end_of_issued()
{
  QueryMap::iterator e = m_queries.lower_bound(m_issuedrange.second);
  assert((e == m_queries.end()) || (e->first >= m_issuedrange.second));
  return e;
}


void pqxx::pipeline::issue(pipeline::QueryMap::const_iterator stop)
{
  assert(m_num_waiting);
  assert(!have_pending());
  assert(!m_dummy_pending);
  assert(m_num_waiting);
  invariant();

  check_end_results();

  // Start with oldest query (lowest id) not in previous issue range
  const QueryMap::const_iterator oldest = end_of_issued();

  if ((oldest != m_queries.end()) && (m_error <= oldest->first)) return;

  assert(oldest != m_queries.end());
  assert(distance(oldest, stop) >= 0);


  string cum;
  QueryMap::const_iterator i;
  for (i = oldest; (i != stop) && (i->first < m_error); ++i)
  {
    cum += i->second.get_query();
    cum += separator();
  }
  const int num_issued = distance(oldest, i);
  const bool prepend_dummy = (num_issued > 1);
  if (prepend_dummy) cum = string("SELECT ") + dummyvalue() + separator() + cum;

  m_Trans.start_exec(cum);

  // Since we managed to send out these queries, update state to reflect this
  m_dummy_pending = prepend_dummy;
  m_issuedrange.first = oldest->first;
  if (stop == m_queries.end())
    m_issuedrange.second = m_queries.rbegin()->first + 1;
  else
    m_issuedrange.second = stop->first;
  m_num_waiting -= num_issued;

  invariant();
}


void pqxx::pipeline::internal_error(const string &err) throw (logic_error)
{
  set_error_at(0);
  throw logic_error(err);
}


bool pqxx::pipeline::obtain_result(bool really_expect)
{
  assert(!m_dummy_pending);
  assert(have_pending() || !really_expect);
  invariant();

  PGresult *r = m_Trans.get_result();
  if (!r)
  {
    if (really_expect) set_error_at(m_issuedrange.first);
    return false;
  }

  // Must be the result for the oldest pending query
  const QueryMap::iterator q = oldest_issued();
  if (!q->second.get_result().empty())
    internal_error("libpqxx internal error: multiple results for one query");

  // Move starting point of m_issuedrange ahead to next-oldest pending query
  // (if there is one; if not, set to end of range)
  QueryMap::const_iterator suc = q;
  ++suc;
  if (suc != m_queries.end()) m_issuedrange.first = suc->first;
  else m_issuedrange.first = m_issuedrange.second;

  q->second.set_result(result(r));

  invariant();

  return true;
}


void pqxx::pipeline::obtain_dummy()
{
  assert(m_dummy_pending);
  PGresult *const r = m_Trans.get_result();
  m_dummy_pending = false;

  if (!r) 
    internal_error("libpqxx internal error: "
	  "pipeline got no result from backend when it expected one");

  result R(r);
  bool OK = false;
  try
  {
    R.CheckStatus("");
    OK = true;
  }
  catch (const sql_error &)
  {
  }
  if (OK)
  {
    if (R.size() > 1)
      internal_error("libpqxx internal error: "
	  "unexpected result for dummy query in pipeline");

    if (string(R.at(0).at(0).c_str()) != dummyvalue())
      internal_error("libpqxx internal error: "
	    "dummy query in pipeline returned unexpected value");
    return;
  }

  /* Since none of the queries in the batch were actually executed, we can
   * afford to replay them one by one until we find the exact query that
   * caused the error.  This gives us not only a more specific error message
   * to report, but also tells us which query to report it for.
   */
  QueryMap::iterator q = oldest_issued();
  const QueryMap::iterator stop = end_of_issued();

  assert(q->first == m_issuedrange.first);

  // First, give the whole batch the same syntax error message, in case all else
  // is going to fail.
  for (QueryMap::iterator i = q; i != stop; ++i) i->second.set_result(R);

  check_end_results();

  // Reset internal state to forget botched batch attempt
  m_num_waiting += distance(q, stop);
  m_issuedrange.second = m_issuedrange.first;

  assert(q->first == m_issuedrange.first);
  assert(end_of_issued() == oldest_issued());
  assert(!m_dummy_pending);
  assert(!have_pending());
  assert(m_num_waiting > 0);

  // Issue queries in failed batch one at a time.
  unregister_me();
  try
  {
    do 
    {
      m_num_waiting--;
      q->second.set_result(m_Trans.exec(q->second.get_query()));
      q->second.get_result().CheckStatus(q->second.get_query());
      q++;
    }
    while (q != m_queries.end());
  }
  catch (const exception &e)
  {
    assert(q != m_queries.end());

    query_id thud = q->first + 1;
    const QueryMap::const_iterator allstop = m_queries.lower_bound(thud);
    if (allstop != m_queries.end()) thud = allstop->first;
    set_error_at(thud);
    m_issuedrange.first = m_issuedrange.second = thud;

  //  assert(m_num_waiting == distance(q, m_queries.end()));
  }
  register_me();
  assert(q != m_queries.end());
  assert(m_error < qid_limit());
}


pair<pipeline::query_id, result>
pqxx::pipeline::retrieve(pipeline::QueryMap::iterator q)
{
  invariant();

  if (q == m_queries.end())
    throw logic_error("Attempt to retrieve result for unknown query");

  // TODO: Wish we could make this error a bit clearer!
  if (q->first >= m_error)
    throw runtime_error("Could not complete query in pipeline "
	"due to error in earlier query");

  QueryMap::const_iterator suc = q;
  ++suc;

  if (q->first >= m_issuedrange.second) 
  {
    // Desired query hasn't issued yet
    if (have_pending()) receive(end_of_issued());
    issue();
  }
  if (q->first >= m_issuedrange.first) receive(suc);
  if (m_num_waiting && !have_pending()) issue();

  const string query(q->second.get_query());
  const result R = q->second.get_result();
  pair<query_id,result> P(make_pair(q->first, R));
  m_queries.erase(q);

  invariant();

  R.CheckStatus(query);
  return P;
}


void pqxx::pipeline::get_further_available_results()
{
  assert(!m_dummy_pending);
  while (have_pending() && !m_Trans.is_busy() && obtain_result(true));
  if (!have_pending()) check_end_results();
}


void pqxx::pipeline::receive_if_available()
{
  invariant();

  m_Trans.consume_input();
  if (m_Trans.is_busy()) return;

  if (m_dummy_pending) obtain_dummy();
  if (have_pending()) get_further_available_results();

  invariant();
}


void pqxx::pipeline::receive(pipeline::QueryMap::const_iterator stop)
{
  invariant();
  assert(have_pending());

  if (m_dummy_pending) obtain_dummy();

  QueryMap::const_iterator q = oldest_issued();
  while (have_pending() && (q != stop) && obtain_result(true)) ++q;

  // Also haul in any remaining "targets of opportunity"
  if (have_pending()) get_further_available_results();

  // Get that terminating NULL result
  if (!have_pending()) check_end_results();
}


void pqxx::pipeline::check_end_results()
{
  if (obtain_result(false))
  {
    set_error_at(m_queries.begin()->first);
    throw logic_error("Got more results from pipeline than there were queries");
  }
}

