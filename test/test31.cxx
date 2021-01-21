#include <cstdio>
#include <iostream>
#include <vector>

#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  See which fields in a query are null, and figure
// out whether any fields are lexicographically sorted.
namespace
{
template<typename VEC, typename VAL> void InitVector(VEC &V, int s, VAL val)
{
  V.resize(static_cast<std::size_t>(s));
  for (auto i{std::begin(V)}; i != std::end(V); ++i) *i = val;
}


void test_031()
{
  connection conn;

  std::string const Table{"pg_tables"};

  std::vector<int> NullFields;            // Maps column to no. of null fields
  std::vector<bool> SortedUp, SortedDown; // Does column appear to be sorted?

  work tx(conn, "test31");

  result R(tx.exec("SELECT * FROM " + Table));

  InitVector(NullFields, R.columns(), 0);
  InitVector(SortedUp, R.columns(), true);
  InitVector(SortedDown, R.columns(), true);

  for (auto i{std::begin(R)}; i != std::end(R); i++)
  {
    PQXX_CHECK_EQUAL(
      (*i).rownumber(), i->rownumber(),
      "operator*() is inconsistent with operator->().");

    PQXX_CHECK_EQUAL(
      i->size(), R.columns(),
      "Row size is inconsistent with result::columns().");

    // Look for null fields
    for (pqxx::row::size_type f{0}; f < i->size(); ++f)
    {
      auto const offset{static_cast<std::size_t>(f)};
      NullFields[offset] += int{i.at(f).is_null()};

      std::string A, B;
      PQXX_CHECK_EQUAL(
        i[f].to(A), i[f].to(B, std::string{}),
        "Variants of to() disagree on nullness.");

      PQXX_CHECK_EQUAL(A, B, "Variants of to() produce different values.");
    }

    // Compare fields to those of preceding row
    if (i != std::begin(R))
    {
      auto const j{i - 1};

      // First perform some sanity checks on j vs. i and how libpqxx handles
      // their interrelationship...
      PQXX_CHECK_EQUAL(i - j, 1, "Iterator successor is at wrong distance.");

      PQXX_CHECK_NOT_EQUAL(j, i, "Iterator equals successor.");
      PQXX_CHECK(j != i, "Iterator is not different from successor.");
      PQXX_CHECK(not(j >= i), "Iterator does not precede successor.");
      PQXX_CHECK(not(j > i), "Iterator follows successor.");
      PQXX_CHECK(not(i <= j), "operator<=() is asymmetric.");
      PQXX_CHECK(not(i < j), "operator<() is asymmetric.");
      PQXX_CHECK(j <= i, "operator<=() is inconsistent.");
      PQXX_CHECK(j < i, "operator<() is inconsistent.");

      PQXX_CHECK_EQUAL(1 + j, i, "Predecessor+1 brings us to wrong place.");

      result::const_iterator k(i);
      PQXX_CHECK_EQUAL(k--, i, "Post-decrement returns wrong value.");
      PQXX_CHECK_EQUAL(k, j, "Post-decrement goes to wrong position.");

      result::const_iterator l(i);
      PQXX_CHECK_EQUAL(--l, j, "Pre-decrement returns wrong value.");
      PQXX_CHECK_EQUAL(l, j, "Pre-decrement goes to wrong position.");

      PQXX_CHECK_EQUAL(k += 1, i, "operator+=() returns wrong value.");
      PQXX_CHECK_EQUAL(k, i, "operator+=() goes to wrong position.");

      PQXX_CHECK_EQUAL(k -= 1, j, "operator-=() returns wrong value.");
      PQXX_CHECK_EQUAL(k, j, "operator-=() goes to wrong position.");

      // ...Now let's do meaningful stuff with j, such as finding out which
      // fields may be sorted.  Don't do anything fancy like trying to
      // detect numbers and comparing them as such, just compare them as
      // simple strings.
      for (pqxx::row::size_type f{0}; f < R.columns(); ++f)
      {
        auto const offset{static_cast<std::size_t>(f)};
        if (not j[f].is_null())
        {
          bool const U{SortedUp[offset]}, D{SortedDown[offset]};

          SortedUp[offset] =
            U & (std::string{j[f].c_str()} <= std::string{i[f].c_str()});
          SortedDown[offset] =
            D & (std::string{j[f].c_str()} >= std::string{i[f].c_str()});
        }
      }
    }
  }

  for (pqxx::row::size_type f{0}; f < R.columns(); ++f)
    PQXX_CHECK_BOUNDS(
      NullFields[static_cast<std::size_t>(f)], 0, internal::ssize(R) + 1,
      "Found more nulls than there were rows.");
}


PQXX_REGISTER_TEST(test_031);
} // namespace
