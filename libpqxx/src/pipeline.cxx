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

#include "pqxx/pipeline"

using namespace PGSTD;


pqxx::pipeline::pipeline(transaction_base &t) :
  m_home(t),
  m_queries(),
  m_waiting(),
  m_completed(),
  m_nextid(1),
  m_retain(false)
{
}


pqxx::pipeline::~pipeline()
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

  // Reduce the risk of exceptions occurring between our updates
  m_waiting.reserve(m_waiting.size()+1);
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


pair<pqxx::pipeline::query_id, pqxx::result> 
pqxx::pipeline::deliver(map<query_id, pqxx::result>::iterator i)
{
  if (i == m_completed.end())
    throw logic_error("libpqxx internal error: delivering from empty pipeline");

  pair<query_id, result> out = *i;
  m_completed.erase(i);
  const map<query_id, string>::iterator q = m_queries.find(out.first);
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


pair<pqxx::pipeline::query_id, pqxx::result> pqxx::pipeline::retrieve()
{
  if (m_completed.empty())
  {
    if (m_sent.empty() && m_waiting.empty())
      throw logic_error("Attempt to retrieve result from empty query pipeline");
    resume();
    consumeresults();

    if (m_completed.empty())
      throw logic_error("libpqxx internal error: no results in pipeline");
  }

  return deliver(m_completed.begin());
}


pqxx::result pqxx::pipeline::retrieve(pqxx::pipeline::query_id qid)
{
  map<query_id, result>::iterator c = m_completed.find(qid);
  if (c == m_completed.end())
  {
    if (!m_sent.empty()) consumeresults();
    c = m_completed.find(qid);
    if (c == m_completed.end()) resume();
    c = m_completed.find(qid);
    if (c == m_completed.end())
      throw logic_error("Attempt to retrieve result for unknown query " +
	  ToString(qid) + " from pipeline");
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


void pqxx::pipeline::attach()
{
  // TODO: TODO: TODO:
}


void pqxx::pipeline::detach()
{
  // TODO: TODO: TODO:
}


void pqxx::pipeline::send_waiting()
{
  if (m_waiting.empty() || !m_sent.empty() || m_retain) return;

  static const string Separator = "; ";
  string Cum;

  for (vector<query_id>::const_iterator i = m_waiting.begin(); 
       i != m_waiting.end(); 
       ++i)
  {
    map<query_id, string>::const_iterator q = m_queries.find(*i);
    if (q == m_queries.end())
      throw logic_error("libpqxx internal error: unknown query issued");
    Cum += q->second;
    Cum += Separator;
  }
  Cum.resize(Cum.size()-Separator.size());

  m_home.start_exec(Cum);
  m_sent.swap(m_waiting);
  attach();
}


void pqxx::pipeline::consumeresults()
{
  if (m_waiting.empty() && m_sent.empty()) return;
  send_waiting();

  vector<result> R;
  R.reserve(m_sent.size());
  for (PGresult *r=m_home.get_result(); r; r=m_home.get_result())
    R.push_back(result(r));

  detach();

  // TODO: If one part of the query fails, the whole thing seems to fail!
  if (R.size() > m_sent.size())
    throw logic_error("libpqxx internal error: "
	              "expected " + ToString(m_sent.size()) + " results "
		      "from pipeline, got " + ToString(R.size()));

  // TODO: Make this exception-safe if possible
  for (vector<result>::size_type i=0; i<R.size(); ++i)
    m_completed.insert(make_pair(m_sent[i], R[i]));
  m_sent.clear();

  send_waiting();
}


void pqxx::pipeline::resume()
{
  m_retain = false;
  send_waiting();
}

