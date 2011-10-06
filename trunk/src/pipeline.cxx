/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pipeline.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::pipeline class
 *   Throughput-optimized query manager
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

#ifdef PQXX_QUIET_DESTRUCTORS
#include "pqxx/errorhandler"
#endif

#include "pqxx/dbtransaction"
#include "pqxx/pipeline"

#include "pqxx/internal/gates/connection-pipeline.hxx"
#include "pqxx/internal/gates/result-creation.hxx"


using namespace PGSTD;
using namespace pqxx;
using namespace pqxx::internal;

#define pqxxassert(ARG) /* ignore */

namespace
{
const string theSeparator("; ");
const string theDummyValue("1");
const string theDummyQuery("SELECT " + theDummyValue + theSeparator);
}


pqxx::pipeline::pipeline(transaction_base &t, const PGSTD::string &Name) :
  namedclass("pipeline", Name),
  transactionfocus(t),
  m_queries(),
  m_issuedrange(),
  m_retain(0),
  m_num_waiting(0),
  m_q_id(0),
  m_dummy_pending(false),
  m_error(qid_limit())
{
  m_issuedrange = make_pair(m_queries.end(), m_queries.end());
  attach();
}


pqxx::pipeline::~pipeline() throw ()
{
#ifdef PQXX_QUIET_DESTRUCTORS
  quiet_errorhandler quiet(m_Trans.conn());
#endif
  try { cancel(); } catch (const exception &) {}
  detach();
}


void pqxx::pipeline::attach()
{
  if (!registered()) register_me();
}


void pqxx::pipeline::detach()
{
  if (registered()) unregister_me();
}


pipeline::query_id pqxx::pipeline::insert(const PGSTD::string &q)
{
  attach();
  const query_id qid = generate_id();
  pqxxassert(qid > 0);
  pqxxassert(m_queries.lower_bound(qid)==m_queries.end());
  const QueryMap::iterator i = m_queries.insert(make_pair(qid,Query(q))).first;

  if (m_issuedrange.second == m_queries.end())
  {
    m_issuedrange.second = i;
    if (m_issuedrange.first == m_queries.end()) m_issuedrange.first = i;
  }
  m_num_waiting++;

  pqxxassert(m_issuedrange.first != m_queries.end());
  pqxxassert(m_issuedrange.second != m_queries.end());

  if (m_num_waiting > m_retain)
  {
    if (have_pending()) receive_if_available();
    if (!have_pending()) issue();
  }

  return qid;
}


void pqxx::pipeline::complete()
{
  if (have_pending()) receive(m_issuedrange.second);
  if (m_num_waiting && (m_error == qid_limit()))
  {
    pqxxassert(!have_pending());
    issue();
    pqxxassert(!m_num_waiting);
    pqxxassert(have_pending());
    pqxxassert(m_issuedrange.second == m_queries.end());
    receive(m_queries.end());
    pqxxassert((m_error!=qid_limit()) || !have_pending());
  }
  detach();
  pqxxassert((m_num_waiting == 0) || (m_error != qid_limit()));
  pqxxassert(!m_dummy_pending);
}


void pqxx::pipeline::flush()
{
  if (!m_queries.empty())
  {
    if (have_pending()) receive(m_issuedrange.second);
    m_issuedrange.first = m_issuedrange.second = m_queries.end();
    m_num_waiting = 0;
    m_dummy_pending = false;
    m_queries.clear();
  }
  detach();
}


void pqxx::pipeline::cancel()
{
  while (have_pending())
  {
    gate::connection_pipeline(m_Trans.conn()).cancel_query();
    QueryMap::iterator canceled_query = m_issuedrange.first;
    ++m_issuedrange.first;
    m_queries.erase(canceled_query);
  }
}


bool pqxx::pipeline::is_finished(pipeline::query_id q) const
{
  if (m_queries.find(q) == m_queries.end())
    throw logic_error("Requested status for unknown query " + to_string(q));
  return (QueryMap::const_iterator(m_issuedrange.first)==m_queries.end()) ||
         (q < m_issuedrange.first->first && q < m_error);
}


pair<pipeline::query_id, result> pqxx::pipeline::retrieve()
{
  if (m_queries.empty())
    throw logic_error("Attempt to retrieve result from empty pipeline");
  return retrieve(m_queries.begin());
}


int pqxx::pipeline::retain(int retain_max)
{
  if (retain_max < 0)
    throw range_error("Attempt to make pipeline retain " +
	to_string(retain_max) + " queries");

  const int oldvalue = m_retain;
  m_retain = retain_max;

  if (m_num_waiting >= m_retain) resume();

  return oldvalue;
}


void pqxx::pipeline::resume()
{
  if (have_pending()) receive_if_available();
  if (!have_pending() && m_num_waiting)
  {
    issue();
    receive_if_available();
  }
}


pipeline::query_id pqxx::pipeline::generate_id()
{
  if (m_q_id == qid_limit())
    throw overflow_error("Too many queries went through pipeline");
  ++m_q_id;
  return m_q_id;
}



void pqxx::pipeline::issue()
{
  pqxxassert(m_num_waiting);
  pqxxassert(!have_pending());
  pqxxassert(!m_dummy_pending);
  pqxxassert(m_num_waiting);

  // TODO: Wrap in nested transaction if available, for extra "replayability"

  // Retrieve that NULL result for the last query, if needed
  obtain_result();

  // Don't issue anything if we've encountered an error
  if (m_error < qid_limit()) return;

  // Start with oldest query (lowest id) not in previous issue range
  QueryMap::iterator oldest = m_issuedrange.second;
  pqxxassert(oldest != m_queries.end());

  // Construct cumulative query string for entire batch
  string cum = separated_list(theSeparator,oldest,m_queries.end(),getquery());
  const QueryMap::size_type num_issued =
    QueryMap::size_type(internal::distance(oldest, m_queries.end()));
  const bool prepend_dummy = (num_issued > 1);
  if (prepend_dummy) cum = theDummyQuery + cum;

  gate::connection_pipeline(m_Trans.conn()).start_exec(cum);

  // Since we managed to send out these queries, update state to reflect this
  m_dummy_pending = prepend_dummy;
  m_issuedrange.first = oldest;
  m_issuedrange.second = m_queries.end();
  m_num_waiting -= int(num_issued);
}


void pqxx::pipeline::internal_error(const PGSTD::string &err)
	throw (PGSTD::logic_error)
{
  set_error_at(0);
  throw pqxx::internal_error(err);
}


bool pqxx::pipeline::obtain_result(bool expect_none)
{
  pqxxassert(!m_dummy_pending);
  pqxxassert(!m_queries.empty());

  gate::connection_pipeline gate(m_Trans.conn());
  internal::pq::PGresult *r = gate.get_result();
  if (!r)
  {
    if (have_pending() && !expect_none)
    {
      set_error_at(m_issuedrange.first->first);
      m_issuedrange.second = m_issuedrange.first;
    }
    return false;
  }

  pqxxassert(r);
  const result res = gate::result_creation::create(
	r,
	0,
	m_queries.begin()->second.get_query(),
	gate::connection_pipeline(m_Trans.conn()).encoding_code());

  if (!have_pending())
  {
    set_error_at(m_queries.begin()->first);
    throw logic_error("Got more results from pipeline than there were queries");
  }

  // Must be the result for the oldest pending query
  if (!m_issuedrange.first->second.get_result().empty())
    internal_error("multiple results for one query");

  m_issuedrange.first->second.set_result(res);
  ++m_issuedrange.first;

  return true;
}


void pqxx::pipeline::obtain_dummy()
{
  pqxxassert(m_dummy_pending);
  gate::connection_pipeline gate(m_Trans.conn());
  internal::pq::PGresult *const r = gate.get_result();
  m_dummy_pending = false;

  if (!r)
    internal_error("pipeline got no result from backend when it expected one");

  result R = gate::result_creation::create(
	r,
	0,
	"[DUMMY PIPELINE QUERY]",
	gate::connection_pipeline(m_Trans.conn()).encoding_code());

  bool OK = false;
  try
  {
    gate::result_creation(R).CheckStatus();
    OK = true;
  }
  catch (const sql_error &)
  {
  }
  if (OK)
  {
    if (R.size() > 1)
      internal_error("unexpected result for dummy query in pipeline");

    if (string(R.at(0).at(0).c_str()) != theDummyValue)
      internal_error("dummy query in pipeline returned unexpected value");
    return;
  }

  /* Since none of the queries in the batch were actually executed, we can
   * afford to replay them one by one until we find the exact query that
   * caused the error.  This gives us not only a more specific error message
   * to report, but also tells us which query to report it for.
   */
  // First, give the whole batch the same syntax error message, in case all else
  // is going to fail.
  for (QueryMap::iterator i = m_issuedrange.first;
       i != m_issuedrange.second;
       ++i) i->second.set_result(R);

  // Remember where the end of this batch was
  const QueryMap::iterator stop = m_issuedrange.second;

  // Retrieve that NULL result for the last query, if needed
  obtain_result(true);


  // Reset internal state to forget botched batch attempt
  m_num_waiting += int(internal::distance(m_issuedrange.first, stop));
  m_issuedrange.second = m_issuedrange.first;

  pqxxassert(!m_dummy_pending);
  pqxxassert(!have_pending());
  pqxxassert(m_num_waiting > 0);

  // Issue queries in failed batch one at a time.
  unregister_me();
  try
  {
    do
    {
      m_num_waiting--;
      const string &query = m_issuedrange.first->second.get_query();
      const result res(m_Trans.exec(query));
      m_issuedrange.first->second.set_result(res);
      gate::result_creation(res).CheckStatus();
      ++m_issuedrange.first;
    }
    while (m_issuedrange.first != stop);
  }
  catch (const exception &)
  {
    pqxxassert(m_issuedrange.first != m_queries.end());

    const query_id thud = m_issuedrange.first->first;
    ++m_issuedrange.first;
    m_issuedrange.second = m_issuedrange.first;
    QueryMap::const_iterator q = m_issuedrange.first;
    set_error_at( (q == m_queries.end()) ?  thud + 1 : q->first);

    pqxxassert(m_num_waiting ==
      internal::distance(m_issuedrange.second, m_queries.end()));
  }

  pqxxassert(m_issuedrange.first != m_queries.end());
  pqxxassert(m_error <= m_q_id);
}


PGSTD::pair<pipeline::query_id, result>
pqxx::pipeline::retrieve(pipeline::QueryMap::iterator q)
{
  if (q == m_queries.end())
    throw logic_error("Attempt to retrieve result for unknown query");

  if (q->first >= m_error)
    throw runtime_error("Could not complete query in pipeline "
	"due to error in earlier query");

  // If query hasn't issued yet, do it now
  if (m_issuedrange.second != m_queries.end() &&
      (q->first >= m_issuedrange.second->first))
  {
    pqxxassert(internal::distance(m_issuedrange.second, q) >= 0);

    if (have_pending()) receive(m_issuedrange.second);
    if (m_error == qid_limit()) issue();
  }

  // If result not in yet, get it; else get at least whatever's convenient
  if (have_pending())
  {
    if (q->first >= m_issuedrange.first->first)
    {
      QueryMap::const_iterator suc = q;
      ++suc;
      receive(suc);
    }
    else
    {
      receive_if_available();
    }
  }

  pqxxassert((m_error <= q->first) || (q != m_issuedrange.first));

  if (q->first >= m_error)
    throw runtime_error("Could not complete query in pipeline "
	"due to error in earlier query");

  // Don't leave the backend idle if there are queries waiting to be issued
  if (m_num_waiting && !have_pending() && (m_error==qid_limit())) issue();

  const result R = q->second.get_result();
  pair<query_id,result> P(make_pair(q->first, R));

  m_queries.erase(q);

  gate::result_creation(R).CheckStatus();
  return P;
}


void pqxx::pipeline::get_further_available_results()
{
  pqxxassert(!m_dummy_pending);
  gate::connection_pipeline gate(m_Trans.conn());
  while (!gate.is_busy() && obtain_result())
    if (!gate.consume_input()) throw broken_connection();
}


void pqxx::pipeline::receive_if_available()
{
  gate::connection_pipeline gate(m_Trans.conn());
  if (!gate.consume_input()) throw broken_connection();
  if (gate.is_busy()) return;

  if (m_dummy_pending) obtain_dummy();
  if (have_pending()) get_further_available_results();
}


void pqxx::pipeline::receive(pipeline::QueryMap::const_iterator stop)
{
  pqxxassert(have_pending());

  if (m_dummy_pending) obtain_dummy();

  while (obtain_result() &&
         QueryMap::const_iterator(m_issuedrange.first) != stop) ;

  // Also haul in any remaining "targets of opportunity"
  if (QueryMap::const_iterator(m_issuedrange.first) == stop)
    get_further_available_results();
}


