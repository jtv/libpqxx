/** Implementation of the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "pqxx/internal/header-pre.hxx"

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/except.hxx"
#include "pqxx/internal/result_iterator.hxx"
#include "pqxx/result.hxx"
#include "pqxx/row.hxx"

#include "pqxx/internal/header-post.hxx"


namespace pqxx
{
PQXX_DECLARE_ENUM_CONVERSION(ExecStatusType);
} // namespace pqxx

std::string const pqxx::result::s_empty_string;


/// C++ wrapper for libpq's PQclear.
void pqxx::internal::clear_result(pq::PGresult const *data) noexcept
{
  // This acts as a destructor, though implemented as a regular function so we
  // can pass it into a smart pointer.  That's why I think it's kind of fair
  // to treat the PGresult as const.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  PQclear(const_cast<pq::PGresult *>(data));
}


pqxx::result::result(
  std::shared_ptr<pqxx::internal::pq::PGresult> const &rhs,
  std::shared_ptr<std::string> const &query,
  std::shared_ptr<pqxx::internal::notice_waiters> const &notice_waiters,
  encoding_group enc) :
        m_data{rhs},
        m_query{query},
        m_notice_waiters{notice_waiters},
        m_encoding(enc)
{}


pqxx::result::const_reverse_iterator pqxx::result::rbegin() const noexcept
{
  return const_reverse_iterator{end()};
}


pqxx::result::const_reverse_iterator pqxx::result::crbegin() const noexcept
{
  return rbegin();
}


pqxx::result::const_reverse_iterator pqxx::result::rend() const noexcept
{
  return const_reverse_iterator{begin()};
}


pqxx::result::const_reverse_iterator pqxx::result::crend() const noexcept
{
  return rend();
}


pqxx::result::const_iterator pqxx::result::begin() const noexcept
{
  return {*this, 0};
}


pqxx::result::const_iterator pqxx::result::cbegin() const noexcept
{
  return begin();
}


pqxx::result::size_type pqxx::result::size() const noexcept
{
  return (m_data.get() == nullptr) ?
           0 :
           static_cast<size_type>(PQntuples(m_data.get()));
}


bool pqxx::result::empty() const noexcept
{
  return (m_data.get() == nullptr) or (PQntuples(m_data.get()) == 0);
}


pqxx::row_ref pqxx::result::front() const noexcept
{
  return {*this, 0};
}


pqxx::row_ref pqxx::result::back() const noexcept
{
  return {*this, size() - 1};
}


void pqxx::result::swap(result &rhs) noexcept
{
  m_data.swap(rhs.m_data);
  m_query.swap(rhs.m_query);
}


pqxx::row_ref pqxx::result::operator[](result_size_type i) const noexcept
{
  return {*this, i};
}


#if defined(PQXX_HAVE_MULTIDIM)
pqxx::field_ref pqxx::result::operator[](
  result_size_type row_num, row_size_type col_num) const noexcept
{
  return {*this, row_num, col_num};
}
#endif // PQXX_HAVE_MULTIDIM


pqxx::row_ref pqxx::result::at(pqxx::result::size_type i, sl loc) const
{
  if (std::cmp_less(i, 0) or std::cmp_greater_equal(i, size()))
    throw range_error{"Row number out of range.", loc};
  return operator[](i);
}


pqxx::field_ref pqxx::result::at(
  pqxx::result_size_type row_num, pqxx::row_size_type col_num, sl loc) const
{
  if (std::cmp_less(row_num, 0) or std::cmp_greater_equal(row_num, size()))
    throw range_error{"Row number out of range.", loc};
  if (std::cmp_less(col_num, 0) or std::cmp_greater_equal(col_num, columns()))
    throw range_error{"Column out of range.", loc};
  return {*this, row_num, col_num};
}


namespace
{
/// C string comparison.
PQXX_ZARGS PQXX_PURE inline bool equal(char const lhs[], char const rhs[])
{
  return strcmp(lhs, rhs) == 0;
}
} // namespace

PQXX_COLD void pqxx::result::throw_sql_error(
  std::string const &Err, std::string const &Query, sl loc) const
{
  // Try to establish more precise error type, and throw corresponding
  // type of exception.
  char const *const code{PQresultErrorField(m_data.get(), PG_DIAG_SQLSTATE)};
  if (code == nullptr)
  {
    // No SQLSTATE at all.  Can this even happen?
    // Let's assume the connection is no longer usable.
    throw broken_connection{Err, loc};
  }

  switch (code[0])
  {
  case '\0':
    // SQLSTATE is empty.  We may have seen this happen in one
    // circumstance: a client-side socket timeout (while using the
    // tcp_user_timeout connection option).  Unfortunately in that case the
    // connection was just fine, so we had no real way of detecting the
    // problem.  (Trying to continue to use the connection does break
    // though, so I feel justified in panicking.)
    throw broken_connection{Err, loc};

  case '0':
    switch (code[1])
    {
    case 'A': throw feature_not_supported{Err, Query, code, loc};
    case '8':
      if (equal(code, "08P01"))
        throw protocol_violation{Err, Query, code, loc};
      throw broken_connection{Err, loc};
    case 'L':
    case 'P': throw insufficient_privilege{Err, Query, code, loc};
    }
    break;
  case '2':
    switch (code[1])
    {
    case '2': throw data_exception{Err, Query, code, loc};
    case '3':
      if (equal(code, "23001"))
        throw restrict_violation{Err, Query, code, loc};
      if (equal(code, "23502"))
        throw not_null_violation{Err, Query, code, loc};
      if (equal(code, "23503"))
        throw foreign_key_violation{Err, Query, code, loc};
      if (equal(code, "23505"))
        throw unique_violation{Err, Query, code, loc};
      if (equal(code, "23514"))
        throw check_violation{Err, Query, code, loc};
      throw integrity_constraint_violation{Err, Query, code, loc};
    case '4': throw invalid_cursor_state{Err, Query, code, loc};
    case '6': throw invalid_sql_statement_name{Err, Query, code, loc};
    }
    break;
  case '3':
    switch (code[1])
    {
    case '4': throw invalid_cursor_name{Err, Query, code, loc};
    }
    break;
  case '4':
    switch (code[1])
    {
    case '0':
      if (equal(code, "40000"))
        throw transaction_rollback{Err, Query, code, loc};
      if (equal(code, "40001"))
        throw serialization_failure{Err, Query, code, loc};
      if (equal(code, "40003"))
        throw statement_completion_unknown{Err, Query, code, loc};
      if (equal(code, "40P01"))
        throw deadlock_detected{Err, Query, code, loc};
      break;
    case '2':
      if (equal(code, "42501"))
        throw insufficient_privilege{Err, Query, code, loc};
      if (equal(code, "42601"))
        throw syntax_error{Err, Query, code, errorposition(), loc};
      if (equal(code, "42703"))
        throw undefined_column{Err, Query, code, loc};
      if (equal(code, "42883"))
        throw undefined_function{Err, Query, code, loc};
      if (equal(code, "42P01"))
        throw undefined_table{Err, Query, code, loc};
    }
    break;
  case '5':
    switch (code[1])
    {
    case '3':
      if (equal(code, "53100"))
        throw disk_full{Err, Query, code, loc};
      if (equal(code, "53200"))
        throw server_out_of_memory{Err, Query, code, loc};
      if (equal(code, "53300"))
        throw too_many_connections{Err, loc};
      throw insufficient_resources{Err, Query, code, loc};
    }
    break;

  case 'P':
    if (equal(code, "P0001"))
      throw plpgsql_raise{Err, Query, code, loc};
    if (equal(code, "P0002"))
      throw plpgsql_no_data_found{Err, Query, code, loc};
    if (equal(code, "P0003"))
      throw plpgsql_too_many_rows{Err, Query, code, loc};
    throw plpgsql_error{Err, Query, code};
  }

  // Unknown error code.
  throw sql_error{Err, Query, code, loc};
}

void pqxx::result::check_status(std::string_view desc, sl loc) const
{
  if (auto err{status_error(loc)}; not std::empty(err)) [[unlikely]]
  {
    if (not std::empty(desc))
      err = std::format("Failure during '{}': {}", desc, err);
    throw_sql_error(err, query(), loc);
  }
}


std::string pqxx::result::status_error(sl loc) const
{
  if (m_data.get() == nullptr)
    throw failure{"No result set given.", loc};

  std::string err;

  switch (PQresultStatus(m_data.get()))
  {
  case PGRES_EMPTY_QUERY: // The string sent to the backend was empty.
  case PGRES_COMMAND_OK:  // Successful completion, no result data.
  case PGRES_TUPLES_OK:   // The query successfully executed.
  case PGRES_COPY_OUT:    // Copy Out (from server) data transfer started.
  case PGRES_COPY_IN:     // Copy In (to server) data transfer started.
  case PGRES_COPY_BOTH:   // Copy In/Out.  Used for streaming replication.
    break;

#if defined(LIBPQ_HAS_PIPELINING)
  case PGRES_PIPELINE_SYNC:    // Pipeline mode synchronisation point.
  case PGRES_PIPELINE_ABORTED: // Previous command in pipeline failed.
    throw feature_not_supported{
      "Not supported yet: libpq pipelines.", "", "", loc};
#endif

  case PGRES_BAD_RESPONSE: // The server's response was not understood.
  case PGRES_NONFATAL_ERROR:
  case PGRES_FATAL_ERROR:
    [[unlikely]] err = PQresultErrorMessage(m_data.get());
    break;

  case PGRES_SINGLE_TUPLE:
    throw feature_not_supported{
      "Not supported: single-row mode.", "", "", loc};

  default:
    throw internal_error{
      std::format(
        "pqxx::result: Unrecognized result status code {}",
        to_string(PQresultStatus(m_data.get()))),
      loc};
  }
  return err;
}


char const *pqxx::result::cmd_status() const noexcept
{
  // PQcmdStatus() can't take a PGresult const * because it returns a non-const
  // pointer into the PGresult's data, and that can't be changed without
  // breaking compatibility.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return PQcmdStatus(const_cast<internal::pq::PGresult *>(m_data.get()));
}


std::string const &pqxx::result::query() const & noexcept
{
  return (m_query == nullptr) ? s_empty_string : *m_query;
}


pqxx::oid pqxx::result::inserted_oid(sl loc) const
{
  if (m_data.get() == nullptr)
    throw usage_error{
      "Attempt to read oid of inserted row without an INSERT result", loc};
  return PQoidValue(m_data.get());
}


pqxx::result::size_type pqxx::result::affected_rows() const
{
  // PQcmdTuples() can't take a "PGresult const *" because it returns a
  // non-const pointer into the PGresult's data, and that can't be changed
  // without breaking compatibility.
  //
  // But as far as libpqxx is concerned, it's totally const.
  std::string_view const rows_str{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    PQcmdTuples(const_cast<internal::pq::PGresult *>(m_data.get()))};

  // rows_str may be empty in case the query executed was a `SET <variable> =
  // ''`
  return rows_str.empty() ? 0 : from_string<size_type>(rows_str);
}


char const *pqxx::result::get_value(
  pqxx::result::size_type row, pqxx::row::size_type col) const noexcept
{
  return PQgetvalue(m_data.get(), row, col);
}


bool pqxx::result::get_is_null(
  pqxx::result::size_type row, pqxx::row::size_type col) const noexcept
{
  return PQgetisnull(m_data.get(), row, col) != 0;
}

pqxx::field::size_type pqxx::result::get_length(
  pqxx::result::size_type row, pqxx::row::size_type col) const noexcept
{
  return static_cast<pqxx::field::size_type>(
    PQgetlength(m_data.get(), row, col));
}


pqxx::oid pqxx::result::column_type(row::size_type col_num, sl loc) const
{
  oid const t{PQftype(m_data.get(), col_num)};
  if (t == oid_none)
    throw argument_error{
      std::format(
        "Attempt to retrieve type of nonexistent column {} of query result.",
        col_num),
      loc};
  return t;
}


pqxx::row::size_type pqxx::result::column_number(zview col_name, sl loc) const
{
  auto const n{PQfnumber(m_data.get(), col_name.c_str())};
  if (n == -1)
    throw argument_error{
      std::format("Unknown column name: '{}'.", to_string(col_name)), loc};

  return static_cast<row::size_type>(n);
}


pqxx::oid pqxx::result::column_table(row::size_type col_num, sl loc) const
{
  oid const t{PQftable(m_data.get(), col_num)};

  /* If we get oid_none, it may be because the column is computed, or because
   * we got an invalid row number.
   */
  if (t == oid_none and col_num >= columns())
    throw argument_error{
      std::format(
        "Attempt to retrieve table ID for column {} out of {}.", col_num,
        columns()),
      loc};

  return t;
}


pqxx::row::size_type
pqxx::result::table_column(row::size_type col_num, sl loc) const
{
  auto const n{row::size_type(PQftablecol(m_data.get(), col_num))};
  if (n != 0) [[likely]]
    return n - 1;

  // Failed.  Now find out why, so we can throw a sensible exception.
  auto const col_str{to_string(col_num)};
  if (col_num > columns())
    throw range_error{
      std::format("Invalid column index in table_column(): {}.", col_str),
      loc};

  if (m_data.get() == nullptr)
    throw usage_error{
      std::format(
        "Can't query origin of column {}: result is not initialized.",
        col_str),
      loc};

  throw usage_error{
    std::format(
      "Can't query origin of column {}: not derived from table column.",
      col_str),
    loc};
}


int pqxx::result::errorposition() const
{
  int pos{-1};
  if (m_data.get())
  {
    auto const p{PQresultErrorField(m_data.get(), PG_DIAG_STATEMENT_POSITION)};
    if (p)
      pos = from_string<decltype(pos)>(p);
  }
  return pos;
}


char const *
pqxx::result::column_name(pqxx::row::size_type number, sl loc) const &
{
  auto const n{PQfname(m_data.get(), number)};
  if (n == nullptr) [[unlikely]]
  {
    if (m_data.get() == nullptr)
      throw usage_error{"Queried column name on null result.", loc};
    throw range_error{
      std::format(
        "Invalid column number: {} (maximum is {}).", number, (columns() - 1)),
      loc};
  }
  return n;
}


pqxx::row::size_type pqxx::result::columns() const noexcept
{
  return m_data ? row::size_type(PQnfields(m_data.get())) : 0;
}


int pqxx::result::column_storage(pqxx::row::size_type number, sl loc) const
{
  int const out{PQfsize(m_data.get(), number)};
  if (out == 0)
  {
    auto const sz{this->size()};
    if (std::cmp_less(number, 0) or std::cmp_greater_equal(number, sz))
      throw argument_error{
        std::format(
          "Column number out of range: {} (have 0 - {})", number, sz),
        loc};
    throw failure{
      std::format("Error getting column_storage for column {}.", number), loc};
  }
  return out;
}


int pqxx::result::column_type_modifier(
  pqxx::row::size_type number) const noexcept
{
  return PQfmod(m_data.get(), number);
}


pqxx::row pqxx::result::one_row(sl loc) const
{
  check_one_row(loc);
  return front();
}


pqxx::row_ref pqxx::result::one_row_ref(sl loc) const
{
  check_one_row(loc);
  return front();
}


pqxx::field pqxx::result::one_field(sl loc) const
{
  expect_columns(1, loc);
  return one_row(loc)[0];
}


pqxx::field_ref pqxx::result::one_field_ref(sl loc) const
{
  expect_columns(1, loc);
  return one_row_ref(loc)[0];
}


std::optional<pqxx::row> pqxx::result::opt_row(sl loc) const
{
  auto const sz{size()};
  if (sz > 1)
  {
    // TODO: See whether result contains a generated statement.
    if (not m_query or m_query->empty())
      throw unexpected_rows{
        std::format("Expected at most 1 row from query, got {}.", sz), loc};
    else
      throw unexpected_rows{
        std::format(
          "Expected at most 1 row from query '{}', got {}.", *m_query, sz),
        loc};
  }
  else if (sz == 1)
  {
    return {front()};
  }
  else
  {
    return {};
  }
}


std::optional<pqxx::row_ref> pqxx::result::opt_row_ref(sl loc) const
{
  // TODO: Reduce duplication with opt_row().
  auto const sz{size()};
  if (sz > 1)
  {
    // TODO: See whether result contains a generated statement.
    if (not m_query or m_query->empty())
      throw unexpected_rows{
        std::format("Expected at most 1 row from query, got {}.", sz), loc};
    else
      throw unexpected_rows{
        std::format(
          "Expected at most 1 row from query '{}', got {}.", *m_query, sz),
        loc};
  }
  else if (sz == 1)
  {
    return {front()};
  }
  else
  {
    return {};
  }
}


// const_result_iterator

pqxx::const_result_iterator pqxx::const_result_iterator::operator++(int) &
{
  const_result_iterator old{*this};
  pqxx::internal::gate::row_ref_const_result_iterator{m_row}.offset(1);
  return old;
}


pqxx::const_result_iterator pqxx::const_result_iterator::operator--(int) &
{
  const_result_iterator old{*this};
  pqxx::internal::gate::row_ref_const_result_iterator{m_row}.offset(-1);
  return old;
}


pqxx::result::const_iterator
pqxx::result::const_reverse_iterator::base() const noexcept
{
  iterator_type tmp{*this};
  return ++tmp;
}


pqxx::const_reverse_result_iterator
pqxx::const_reverse_result_iterator::operator++(int) &
{
  const_reverse_result_iterator const tmp{*this};
  iterator_type::operator--();
  return tmp;
}


pqxx::const_reverse_result_iterator
pqxx::const_reverse_result_iterator::operator--(int) &
{
  const_reverse_result_iterator const tmp{*this};
  iterator_type::operator++();
  return tmp;
}
