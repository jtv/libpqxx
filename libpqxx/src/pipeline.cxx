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

#include "pqxx/dbtransaction"
#include "pqxx/pipeline"

// TODO: Limit number of retained queries so we don't wait forever (tunable?)
using namespace PGSTD;


pqxx::pipeline::pipeline(transaction_base &t, const string &PName) :
  pqxx::internal::transactionfocus(t, PName, "pipeline"),
  m_queries(),
  m_waiting(),
  m_completed(),
  m_nextid(1),
  m_retain(false),
  m_error(false)
{
}


pqxx::pipeline::~pipeline() throw ()
{
  try
  {
    flush();
  }
  catch (const exception &)
  {
  }
}


pqxx::pipeline::query_id pqxx::pipeline::insert(const string &Query)
{
  const query_id id = generate_id();

  try
  {
    m_queries.insert(make_pair(id, Query));
    m_waiting.push_back(id);
  }
  catch (const exception &e)
  {
    m_queries.erase(id);
    if (!m_waiting.empty() && (m_waiting[m_waiting.size()-1] == id))
      m_waiting.resize(m_waiting.size()-1);
    throw;
  }

  send_waiting();
  return id;
}


void pqxx::pipeline::complete()
{
  resume();
  while (!m_waiting.empty() && !m_sent.empty()) consumeresults();
}


void pqxx::pipeline::flush()
{
  m_waiting.clear();
  consumeresults();
  m_sent.clear();
  m_completed.clear();
  m_queries.clear();
  m_error = false;
  resume();
}


bool pqxx::pipeline::is_running(pqxx::pipeline::query_id qid) const
{
  return find(m_sent.begin(), m_sent.end(), qid) != m_sent.end();
}


bool pqxx::pipeline::is_finished(query_id qid) const
{
  return m_completed.find(qid) != m_completed.end();
}


pqxx::pipeline::ResultsMap::value_type
pqxx::pipeline::deliver(ResultsMap::iterator i)
{
  if (i == m_completed.end())
  {
    // TODO: Report what went wrong with which query
    // TODO: Throw sql_error
    if (m_error)
      throw runtime_error("Could not get result from pipeline: "
	  "preceding query failed");
    throw logic_error("libpqxx internal error: delivering from empty pipeline");
  }

  const ResultsMap::value_type out(*i);
  m_completed.erase(i);
  const QueryMap::iterator q = m_queries.find(out.first);
  if (q == m_queries.end())
    throw invalid_argument("Unknown query retrieved from pipeline");

  try
  {
    out.second.CheckStatus(q->second);
  }
  catch (const exception &)
  {
    m_queries.erase(q);
    throw;
  }
  m_queries.erase(q);
  return out;
}


pair<pqxx::pipeline::query_id,pqxx::result> pqxx::pipeline::retrieve()
{
  if (m_completed.empty())
  {
    if (m_sent.empty() && m_waiting.empty())
      throw logic_error("Attempt to retrieve query result from empty pipeline");
    resume();
    consumeresults();
  }

  const ResultsMap::value_type out(deliver(m_completed.begin()));
  return pair<query_id,result>(out.first, out.second);
}


pqxx::result pqxx::pipeline::retrieve(pqxx::pipeline::query_id qid)
{
  ResultsMap::iterator c = m_completed.find(qid);
  if (c == m_completed.end())
  {
    if (!m_sent.empty()) consumeresults();
    c = m_completed.find(qid);
    if (c == m_completed.end()) resume();
    c = m_completed.find(qid);
    if (c == m_completed.end())
    {
      if (m_queries.find(qid) == m_queries.end())
        throw logic_error("Attempt to retrieve result for unknown query " +
		to_string(qid) + " from pipeline");
    }
  }

  return deliver(c).second;
}


bool pqxx::pipeline::empty() const throw ()
{
  return m_queries.empty();
}


pqxx::pipeline::query_id pqxx::pipeline::generate_id()
{
  query_id qid;
  for (qid=m_nextid++; m_queries.find(qid)!=m_queries.end(); qid=m_nextid++);
  return qid;
}


void pqxx::pipeline::send_waiting()
{
  if (m_waiting.empty() || !m_sent.empty() || m_retain || m_error) return;

  static const string Separator = "; ";
  string Cum;
  
  /* Bart Samwel's Genius Trick(tm)
   * If we get only a single result, and it represents an error (as will be the
   * case if there's more than one query, for instance) then it may be either a
   * syntax error anywhere in the cumulated query, or it may be a normal error 
   * that happens to occur in the first query.  The difference matters because
   * in the former case we may want to pinpoint the cause of the error.
   *
   * To make sure we can tell the difference if there's more than one query, we
   * prepend one that cannot generate an error.  Now if we get only a single
   * result, we know there's a syntax error.
   */
  if (m_waiting.size() > 1) Cum = "SELECT 0" + Separator;

  for (QueryQueue::const_iterator i=m_waiting.begin(); i!=m_waiting.end(); ++i)
  {
    QueryMap::const_iterator q = m_queries.find(*i);
    if (q == m_queries.end())
      throw logic_error("libpqxx internal error: unknown query issued");
    Cum += q->second;
    Cum += Separator;
  }
  Cum.resize(Cum.size()-Separator.size());

  m_Trans.start_exec(Cum);
  m_sent.swap(m_waiting);
  register_me();
}


void pqxx::pipeline::consumeresults()
{
  if ((m_waiting.empty() && m_sent.empty()) || m_error) return;
  send_waiting();

  vector<result> R;

  // Reduce risk of exceptions at awkward moments
  R.reserve(m_sent.size() + 1);

  // TODO: Improve exception-safety!
  for (PGresult *r=m_Trans.get_result(); r; r=m_Trans.get_result())
    R.push_back(result(r));

  unregister_me();

  // TODO: Don't wait for all results in the batch to come in!
  vector<result>::size_type R_size = R.size(), sentsize = m_sent.size();

  if (!R_size)
    throw logic_error("libpqxx internal error: got no result from pipeline");

  if (R_size > sentsize+1)
    throw logic_error("libpqxx internal error: "
	              "expected " + to_string(sentsize) + " results "
		      "from pipeline, got " + to_string(R_size));

  if ((R_size == 1) && (sentsize > 1))
  {
    /* Syntax error that comes from one of the queries in the batch, and we
     * don't know which (if there was only one in the first place, we'd treat
     * this as a regular result).  Register the same error result for all 
     * results in the batch.
     */
    // TODO: Improve error reporting
    m_error = true;
    /* As a first approximation, just register the same error for all queries in
     * the batch, so at least the user gets the message about what went wrong.
     */
    for (QueryQueue::size_type i=0; i<sentsize; ++i)
      m_completed.insert(make_pair(m_sent[i], R[0]));

    if (!dynamic_cast<dbtransaction *>(&m_Trans))
    {
      /* We're not in a backend transaction, so we can continue to issue
       * transactions.  Execute the queries in the batch one by one to try and
       * pinpoint which was the one containing the error.
       */
      QueryQueue::size_type i;
      try
      {
        for (i=0; i<sentsize; ++i)
	{
	  try
	  {
	    m_completed[m_sent[i]] = m_Trans.exec(m_queries[m_sent[i]]);
	  }
	  catch (const sql_error &)
	  {
	    /* This ought to be our syntax error.  Break out of the loop, 
	     * because the rest of the queries already have their results set to
	     * be the syntax error.  This is what we want.
	     */
	    throw;
	  }
	  catch (const logic_error &e)
	  {
	    // Internal error.  Make sure it gets reported.
	    throw;
	  }
	  catch (const exception &)
	  {
	    /* Some other error.  Since we're doing nice-to-have work, not
	     * anything essential, it's probably best to continue quietly in
	     * hopes of doing some good.
	     */
	  }
	}
      }
      catch (const sql_error &)
      {
	// Okay, looks like we have our m_completed in the desired state now.
      }
    }
  }
  else
  {
    // Normal situation: the first R_size queries were parsed and performed
    if (sentsize > 1)
    {
      // Strip that safe query we prepended to identify syntax errors
      R.erase(R.begin());
      R_size--;
    }
    if (R_size < sentsize) m_error = true;

    // TODO: Exception safety goes to hell here...
    // Promote finished queries (successful or not) to completed
    for (vector<result>::size_type i=0; i<R_size; ++i)
    {
      try
      {
        m_completed.insert(make_pair(m_sent[i], R[i]));
      }
      catch (const exception &)
      {
	/* Oops.  Well, at least try to make sure we don't leave stuff in
	 * m_sent that's now also in m_completed.
	 */
	m_error = true;
	if (i > 0) m_sent.erase(m_sent.begin(), m_sent.begin()+i-1);
	throw;
      }
    }

    // Push back any queries behind the last one to be executed
    try
    {
      m_waiting.insert(m_waiting.begin(), m_sent.begin()+R_size, m_sent.end());
    }
    catch (const exception &)
    {
      // Not much we can do here, except remember we're in an error state
      m_error = true;
      throw;
    }

    // That last result may still be an error...
    if (!m_error)
    {
      try
      {
        R[R_size-1].CheckStatus(m_queries[m_sent[R_size-1]]);
      }
      catch (const exception &)
      {
        m_error = true;
      }
    }
  }
  m_sent.clear();

  send_waiting();
}


void pqxx::pipeline::resume()
{
  m_retain = false;
  send_waiting();
}

