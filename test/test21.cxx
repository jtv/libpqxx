#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>

#include "helpers.hxx"


// Simple test program for libpqxx.  Open a connection to database, start a
// transaction, and perform a query inside it.
namespace
{
void test_021()
{
  pqxx::connection cx;

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
    pqxx::to_string(cx.port()) +
    ", "
    "backendpid=" +
    pqxx::to_string(cx.backendpid()) + "\n");

  pqxx::work tx{cx, "test_021"};

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
    pqxx::to_string(cx.port()) +
    ", "
    "backendpid=" +
    pqxx::to_string(cx.backendpid()) + "\n");

  std::string P;
  pqxx::from_string(cx.port(), P);
  PQXX_CHECK_EQUAL(P, pqxx::to_string(cx.port()));
  PQXX_CHECK_EQUAL(pqxx::to_string(P), P);

  pqxx::result const R(tx.exec("SELECT * FROM pg_tables"));

  tx.process_notice(std::format(
    "{} result row in transaction {}\n", pqxx::to_string(std::size(R)),
    tx.name()));
  tx.commit();
}


PQXX_REGISTER_TEST(test_021);
} // namespace
