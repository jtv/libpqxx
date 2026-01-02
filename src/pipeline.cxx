/** Implementation of the pqxx::pipeline class.
 *
 * Throughput-optimized query interface.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <iterator>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/dbtransaction.hxx"
#include "pqxx/internal/gates/connection-pipeline.hxx"
#include "pqxx/internal/gates/result-creation.hxx"
#include "pqxx/internal/gates/result-pipeline.hxx"
#include "pqxx/pipeline.hxx"
#include "pqxx/separated_list.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals::string_view_literals;


namespace
{
constexpr std::string_view theSeparator{"; "sv}, theDummyValue{"1"sv},
  theDummyQuery{"SELECT 1; "sv};
} // namespace


void pqxx::pipeline::init(sl loc)
{
  m_encoding = m_trans->conn().get_encoding_group(loc);
  m_issuedrange = make_pair(std::end(m_queries), std::end(m_queries));
  attach();
}


pqxx::pipeline::~pipeline() noexcept
{
  sl const loc{m_created_loc};
  try
  {
    cancel(loc);
  }
  catch (std::exception const &)
  {}
  detach();
}


void pqxx::pipeline::attach()
{
  if (not registered())
    register_me();
}


void pqxx::pipeline::detach()
{
  if (registered())
    unregister_me();
}


pqxx::pipeline::query_id pqxx::pipeline::insert(std::string_view q, sl loc) &
{
  attach();
  query_id const qid{generate_id()};
  auto const i{m_queries.insert(std::make_pair(qid, Query(q))).first};

  if (m_issuedrange.second == std::end(m_queries))
  {
    m_issuedrange.second = i;
    if (m_issuedrange.first == std::end(m_queries))
      m_issuedrange.first = i;
  }
  m_num_waiting++;

  if (m_num_waiting > m_retain)
  {
    if (have_pending())
      receive_if_available(loc);
    if (not have_pending())
      issue(loc);
  }

  return qid;
}


void pqxx::pipeline::complete(sl loc)
{
  if (have_pending())
    receive(m_issuedrange.second, loc);
  if (m_num_waiting and (m_error == qid_limit()))
  {
    issue(loc);
    receive(std::end(m_queries), loc);
  }
  detach();
}


void pqxx::pipeline::flush(sl loc)
{
  if (not std::empty(m_queries))
  {
    if (have_pending())
      receive(m_issuedrange.second, loc);
    m_issuedrange.first = m_issuedrange.second = std::end(m_queries);
    m_num_waiting = 0;
    m_dummy_pending = false;
    m_queries.clear();
  }
  detach();
}


PQXX_COLD void pqxx::pipeline::cancel(sl loc)
{
  while (have_pending())
  {
    pqxx::internal::gate::connection_pipeline(m_trans->conn())
      .cancel_query(loc);
    auto canceled_query{m_issuedrange.first};
    ++m_issuedrange.first;
    m_queries.erase(canceled_query);
  }
}


bool pqxx::pipeline::is_finished(pipeline::query_id q) const
{
  if (not m_queries.contains(q))
    throw std::logic_error{
      std::format("Requested status for unknown query '{}'.", q)};
  return (QueryMap::const_iterator(m_issuedrange.first) ==
          std::end(m_queries)) or
         (q < m_issuedrange.first->first and q < m_error);
}


std::pair<pqxx::pipeline::query_id, pqxx::result>
pqxx::pipeline::retrieve(sl loc)
{
  if (std::empty(m_queries))
    throw std::logic_error{"Attempt to retrieve result from empty pipeline."};
  return retrieve(std::begin(m_queries), loc);
}


int pqxx::pipeline::retain(int retain_max) &
{
  if (retain_max < 0)
    throw range_error{
      std::format("Attempt to make pipeline retain {} queries.", retain_max)};

  int const oldvalue{m_retain};
  m_retain = retain_max;

  if (m_num_waiting >= m_retain)
    resume();

  return oldvalue;
}


void pqxx::pipeline::resume(sl loc) &
{
  if (have_pending())
    receive_if_available(loc);
  if (not have_pending() and m_num_waiting)
  {
    issue(loc);
    receive_if_available(loc);
  }
}


pqxx::pipeline::query_id pqxx::pipeline::generate_id()
{
  if (m_q_id == qid_limit())
    throw std::overflow_error{"Too many queries went through pipeline."};
  ++m_q_id;
  return m_q_id;
}


void pqxx::pipeline::issue(sl loc)
{
  // Retrieve that null result for the last query, if needed.
  obtain_result(false, loc);

  // Don't issue anything if we've encountered an error.
  if (m_error < qid_limit())
    return;

  // Start with oldest query (lowest id) not in previous issue range.
  auto oldest{m_issuedrange.second};

  // Construct cumulative query string for entire batch.
  auto cum{separated_list(
    theSeparator, oldest, std::end(m_queries),
    [](QueryMap::const_iterator i) { return *i->second.query; })};
  auto const num_issued{
    QueryMap::size_type(std::distance(oldest, std::end(m_queries)))};
  bool const prepend_dummy{num_issued > 1};
  if (prepend_dummy)
    cum = std::format("{}{}", theDummyQuery, cum);

  pqxx::internal::gate::connection_pipeline{m_trans->conn()}.start_exec(
    cum.c_str());

  // Since we managed to send out these queries, update state to reflect this.
  m_dummy_pending = prepend_dummy;
  m_issuedrange.first = oldest;
  m_issuedrange.second = std::end(m_queries);
  m_num_waiting -= check_cast<int>(num_issued, "pipeline issue()"sv, loc);
}


PQXX_COLD void pqxx::pipeline::internal_error(std::string const &err, sl loc)
{
  set_error_at(0);
  throw pqxx::internal_error{err, loc};
}


bool pqxx::pipeline::obtain_result(bool expect_none, sl loc)
{
  pqxx::internal::gate::connection_pipeline gate{m_trans->conn()};
  std::shared_ptr<pqxx::internal::pq::PGresult> const r{
    gate.get_result(), pqxx::internal::clear_result};
  if (not r)
  {
    if (have_pending() and not expect_none) [[unlikely]]
    {
      set_error_at(m_issuedrange.first->first);
      m_issuedrange.second = m_issuedrange.first;
    }
    return false;
  }

  pqxx::internal::gate::connection_pipeline const pgate{m_trans->conn()};
  auto handler{pgate.get_notice_waiters()};
  result const res{pqxx::internal::gate::result_creation::create(
    r, std::begin(m_queries)->second.query, handler, m_encoding)};

  if (not have_pending()) [[unlikely]]
  {
    set_error_at(std::begin(m_queries)->first);
    throw std::logic_error{
      "Got more results from pipeline than there were queries."};
  }

  // Must be the result for the oldest pending query.
  if (not std::empty(m_issuedrange.first->second.res)) [[unlikely]]
    internal_error("Multiple results for one query.", loc);

  m_issuedrange.first->second.res = res;
  ++m_issuedrange.first;

  return true;
}


void pqxx::pipeline::obtain_dummy(sl loc)
{
  conversion_context const c{{}, loc};

  // Allocate once, re-use across invocations.
  static auto const text{
    std::make_shared<std::string>("[DUMMY PIPELINE QUERY]")};

  pqxx::internal::gate::connection_pipeline gate{m_trans->conn()};
  std::shared_ptr<pqxx::internal::pq::PGresult> const r{
    gate.get_result(), pqxx::internal::clear_result};
  m_dummy_pending = false;

  if (not r) [[unlikely]]
    internal_error(
      "Pipeline got no result from backend when it expected one.", loc);

  pqxx::internal::gate::connection_pipeline const pgate{m_trans->conn()};
  auto handler{pgate.get_notice_waiters()};
  result const R{pqxx::internal::gate::result_creation::create(
    r, text, handler, m_encoding)};

  bool OK{false};
  try
  {
    pqxx::internal::gate::result_creation{R}.check_status(loc);
    OK = true;
  }
  catch (sql_error const &)
  {}
  if (OK) [[likely]]
  {
    if (std::size(R) > 1) [[unlikely]]
      internal_error("Unexpected result for dummy query in pipeline.", loc);

    if (R.at(0).at(0).view() != theDummyValue) [[unlikely]]
      internal_error(
        "Dummy query in pipeline returned unexpected value.", loc);
    return;
  }

  // TODO: Can we actually re-issue statements after a failure?
  /* Execution of this batch failed.
   *
   * When we send multiple statements in one go, the backend treats them as a
   * single transaction.  So the entire batch was effectively rolled back.
   *
   * Since none of the queries in the batch were actually executed, we can
   * afford to replay them one by one until we find the exact query that
   * caused the error.  This gives us not only a more specific error message
   * to report, but also tells us which query to report it for.
   */
  // First, give the whole batch the same syntax error message, in case all
  // else is going to fail.
  for (auto i{m_issuedrange.first}; i != m_issuedrange.second; ++i)
    i->second.res = R;

  // Remember where the end of this batch was
  auto const stop{m_issuedrange.second};

  // Retrieve that null result for the last query, if needed
  obtain_result(true, loc);

  // Reset internal state to forget botched batch attempt
  m_num_waiting += check_cast<int>(
    std::distance(m_issuedrange.first, stop), "pipeline obtain_dummy()"sv,
    loc);
  m_issuedrange.second = m_issuedrange.first;

  // Issue queries in failed batch one at a time.
  unregister_me();
  try
  {
    do {
      m_num_waiting--;
      auto const query{*m_issuedrange.first->second.query};
      auto &holder{m_issuedrange.first->second};
      holder.res = m_trans->exec(query, loc);
      pqxx::internal::gate::result_creation{holder.res}.check_status(loc);
      ++m_issuedrange.first;
    } while (m_issuedrange.first != stop);
  }
  catch (std::exception const &)
  {
    auto const thud{m_issuedrange.first->first};
    ++m_issuedrange.first;
    m_issuedrange.second = m_issuedrange.first;
    auto q{m_issuedrange.first};
    set_error_at((q == std::end(m_queries)) ? thud + 1 : q->first);
  }
}


std::pair<pqxx::pipeline::query_id, pqxx::result>
pqxx::pipeline::retrieve(pipeline::QueryMap::iterator q, sl loc)
{
  if (q == std::end(m_queries))
    throw std::logic_error{"Attempt to retrieve result for unknown query."};

  if (q->first >= m_error)
    throw std::runtime_error{
      "Could not complete query in pipeline due to error in earlier query."};

  // If query hasn't issued yet, do it now.
  if (
    m_issuedrange.second != std::end(m_queries) and
    (q->first >= m_issuedrange.second->first))
  {
    if (have_pending())
      receive(m_issuedrange.second, loc);
    if (m_error == qid_limit())
      issue(loc);
  }

  // If result not in yet, get it; else get at least whatever's convenient.
  if (have_pending())
  {
    if (q->first >= m_issuedrange.first->first)
    {
      auto suc{q};
      ++suc;
      receive(suc, loc);
    }
    else
    {
      receive_if_available(loc);
    }
  }

  if (q->first >= m_error)
    throw std::runtime_error{
      "Could not complete query in pipeline due to error in earlier query."};

  // Don't leave the backend idle if there are queries waiting to be issued.
  if (m_num_waiting and not have_pending() and (m_error == qid_limit()))
    issue(loc);

  // We do a strange dance with the first argument, just so we get the "move"
  // version of std::make_pair().
  auto p{
    std::make_pair(query_id{q->first}, std::move(std::move(q->second.res)))};
  m_queries.erase(q);
  pqxx::internal::gate::result_creation{p.second}.check_status(loc);
  return p;
}


void pqxx::pipeline::get_further_available_results(sl loc)
{
  pqxx::internal::gate::connection_pipeline gate{m_trans->conn()};
  while (not gate.is_busy() and obtain_result(false, loc))
    if (not gate.consume_input())
      throw broken_connection{loc};
}


void pqxx::pipeline::receive_if_available(sl loc)
{
  pqxx::internal::gate::connection_pipeline gate{m_trans->conn()};
  if (not gate.consume_input())
    throw broken_connection{loc};
  if (gate.is_busy())
    return;

  if (m_dummy_pending)
    obtain_dummy(loc);
  if (have_pending())
    get_further_available_results(loc);
}


void pqxx::pipeline::receive(pipeline::QueryMap::const_iterator stop, sl loc)
{
  if (m_dummy_pending)
    obtain_dummy(loc);

  while (obtain_result(false, loc) and
         QueryMap::const_iterator{m_issuedrange.first} != stop);

  // Also haul in any remaining "targets of opportunity".
  if (QueryMap::const_iterator{m_issuedrange.first} == stop)
    get_further_available_results(loc);
}
