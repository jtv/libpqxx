#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open a connection to database, start a
// transaction, and perform a query inside it.
namespace
{
void test_021()
{
  connection cx;

  std::string const HostName{
    ((cx.hostname() == nullptr) ? "<local>" : cx.hostname())};
  cx.process_notice(
    std::string{} + "database=" + cx.dbname() +
    ", "
    "username=" +
    cx.username() +
    ", "
    "hostname=" +
    HostName +
    ", "
    "port=" +
    to_string(cx.port()) +
    ", "
    "backendpid=" +
    to_string(cx.backendpid()) + "\n");

  work tx{cx, "test_021"};

  // By now our connection should really have been created
  cx.process_notice("Printing details on actual connection\n");
  cx.process_notice(
    std::string{} + "database=" + cx.dbname() +
    ", "
    "username=" +
    cx.username() +
    ", "
    "hostname=" +
    HostName +
    ", "
    "port=" +
    to_string(cx.port()) +
    ", "
    "backendpid=" +
    to_string(cx.backendpid()) + "\n");

  std::string P;
  from_string(cx.port(), P);
  PQXX_CHECK_EQUAL(
    P, to_string(cx.port()), "Port string conversion is broken.");
  PQXX_CHECK_EQUAL(to_string(P), P, "Port string conversion is broken.");

  result R(tx.exec("SELECT * FROM pg_tables"));

  tx.process_notice(pqxx::internal::concat(
    to_string(std::size(R)), " result row in transaction ", tx.name(), "\n"));
  tx.commit();
}


PQXX_REGISTER_TEST(test_021);
} // namespace
