#include <pqxx/cursor>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_forward_sql_cursor(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  // Plain owned, scoped, forward-only read-only cursor.
  pqxx::internal::sql_cursor forward(
    tx, "SELECT generate_series(1, 4)", "forward",
    pqxx::cursor_base::forward_only, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, false);

  PQXX_CHECK_EQUAL(forward.pos(), 0);
  PQXX_CHECK_EQUAL(forward.endpos(), -1);

  auto empty_result{forward.empty_result()};
  PQXX_CHECK_EQUAL(std::size(empty_result), 0);

  auto displacement{0};
  auto one{forward.fetch(1, displacement, pqxx::sl::current())};
  PQXX_CHECK_EQUAL(std::size(one), 1);
  PQXX_CHECK_EQUAL(one[0][0].as<std::string>(), "1");
  PQXX_CHECK_EQUAL(displacement, 1);
  PQXX_CHECK_EQUAL(forward.pos(), 1);

  auto offset{forward.move(1, displacement, pqxx::sl::current())};
  PQXX_CHECK_EQUAL(offset, 1);
  PQXX_CHECK_EQUAL(displacement, 1);
  PQXX_CHECK_EQUAL(forward.pos(), 2);
  PQXX_CHECK_EQUAL(forward.endpos(), -1);

  auto row{forward.fetch(0, displacement, pqxx::sl::current())};
  PQXX_CHECK_EQUAL(std::size(row), 0);
  PQXX_CHECK_EQUAL(displacement, 0);
  PQXX_CHECK_EQUAL(forward.pos(), 2);

  row = forward.fetch(0, pqxx::sl::current());
  PQXX_CHECK_EQUAL(std::size(row), 0);
  PQXX_CHECK_EQUAL(forward.pos(), 2);
  PQXX_CHECK_EQUAL(forward.pos(), 2);

  offset = forward.move(1, pqxx::sl::current());
  PQXX_CHECK_EQUAL(offset, 1);
  PQXX_CHECK_EQUAL(forward.pos(), 3);

  row = forward.fetch(1, pqxx::sl::current());
  PQXX_CHECK_EQUAL(std::size(row), 1);
  PQXX_CHECK_EQUAL(forward.pos(), 4);
  PQXX_CHECK_EQUAL(row[0][0].as<std::string>(), "4");

  empty_result = forward.fetch(1, displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(std::size(empty_result), 0, "Got rows at end of cursor");
  PQXX_CHECK_EQUAL(forward.pos(), 5);
  PQXX_CHECK_EQUAL(forward.endpos(), 5);
  PQXX_CHECK_EQUAL(displacement, 1);

  offset = forward.move(5, displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(offset, 0);
  PQXX_CHECK_EQUAL(forward.pos(), 5, "pos() is beyond end");
  PQXX_CHECK_EQUAL(forward.endpos(), 5);
  PQXX_CHECK_EQUAL(displacement, 0);

  // Move through entire result set at once.
  pqxx::internal::sql_cursor forward2(
    tx, "SELECT generate_series(1, 4)", "forward",
    pqxx::cursor_base::forward_only, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, false);

  // Move through entire result set at once.
  offset =
    forward2.move(pqxx::cursor_base::all(), displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(offset, 4);
  PQXX_CHECK_EQUAL(displacement, 5);
  PQXX_CHECK_EQUAL(forward2.pos(), 5);
  PQXX_CHECK_EQUAL(forward2.endpos(), 5);

  pqxx::internal::sql_cursor forward3(
    tx, "SELECT generate_series(1, 4)", "forward",
    pqxx::cursor_base::forward_only, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, false);

  // Fetch entire result set at once.
  auto rows{forward3.fetch(
    pqxx::cursor_base::all(), displacement, pqxx::sl::current())};
  PQXX_CHECK_EQUAL(std::size(rows), 4);
  PQXX_CHECK_EQUAL(displacement, 5);
  PQXX_CHECK_EQUAL(forward3.pos(), 5);
  PQXX_CHECK_EQUAL(forward3.endpos(), 5);

  pqxx::internal::sql_cursor forward_empty(
    tx, "SELECT generate_series(0, -1)", "forward_empty",
    pqxx::cursor_base::forward_only, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, false);

  offset = forward_empty.move(3, displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(forward_empty.pos(), 1);
  PQXX_CHECK_EQUAL(forward_empty.endpos(), 1);
  PQXX_CHECK_EQUAL(displacement, 1);
  PQXX_CHECK_EQUAL(offset, 0);
}


void test_scroll_sql_cursor(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::internal::sql_cursor scroll(
    tx, "SELECT generate_series(1, 10)", "scroll",
    pqxx::cursor_base::random_access, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, false);

  PQXX_CHECK_EQUAL(scroll.pos(), 0);
  PQXX_CHECK_EQUAL(scroll.endpos(), -1);

  auto rows{scroll.fetch(pqxx::cursor_base::next(), pqxx::sl::current())};
  PQXX_CHECK_EQUAL(std::size(rows), 1);
  PQXX_CHECK_EQUAL(scroll.pos(), 1);
  PQXX_CHECK_EQUAL(scroll.endpos(), -1);

  // Turn cursor around.  This is where we begin to feel SQL cursors'
  // semantics: we pre-decrement, ending up on the position in front of the
  // first row and returning no rows.
  rows = scroll.fetch(pqxx::cursor_base::prior(), pqxx::sl::current());
  PQXX_CHECK_EQUAL(std::empty(rows), true);
  PQXX_CHECK_EQUAL(scroll.pos(), 0);
  PQXX_CHECK_EQUAL(scroll.endpos(), -1);

  // Bounce off the left-hand side of the result set.  Can't move before the
  // starting position.
  auto offset{0}, displacement{0};
  offset = scroll.move(-3, displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(offset, 0);
  PQXX_CHECK_EQUAL(displacement, 0);
  PQXX_CHECK_EQUAL(scroll.pos(), 0);
  PQXX_CHECK_EQUAL(scroll.endpos(), -1);

  // Try bouncing off the left-hand side a little harder.  Take 4 paces away
  // from the boundary and run into it.
  offset = scroll.move(4, displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(offset, 4);
  PQXX_CHECK_EQUAL(displacement, 4);
  PQXX_CHECK_EQUAL(scroll.pos(), 4);
  PQXX_CHECK_EQUAL(scroll.endpos(), -1);

  offset = scroll.move(-10, displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(offset, 3);
  PQXX_CHECK_EQUAL(displacement, -4);
  PQXX_CHECK_EQUAL(scroll.pos(), 0);
  PQXX_CHECK_EQUAL(scroll.endpos(), -1);

  rows = scroll.fetch(3, pqxx::sl::current());
  PQXX_CHECK_EQUAL(scroll.pos(), 3);
  PQXX_CHECK_EQUAL(std::size(rows), 3);
  PQXX_CHECK_EQUAL(rows[2][0].as<int>(), 3);
  rows = scroll.fetch(-1, pqxx::sl::current());
  PQXX_CHECK_EQUAL(scroll.pos(), 2);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 2);

  rows = scroll.fetch(1, pqxx::sl::current());
  PQXX_CHECK_EQUAL(scroll.pos(), 3, "Bad pos() after inverse turnaround");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 3);
}


void test_adopted_sql_cursor(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec(
      "DECLARE adopted SCROLL CURSOR FOR "
      "SELECT generate_series(1, 3)")
    .no_rows();
  pqxx::internal::sql_cursor adopted(tx, "adopted", pqxx::cursor_base::owned);
  PQXX_CHECK_EQUAL(adopted.pos(), -1, "Adopted cursor has known pos()");
  PQXX_CHECK_EQUAL(adopted.endpos(), -1, "Adopted cursor has known endpos()");

  auto displacement{0};
  auto rows{adopted.fetch(
    pqxx::cursor_base::all(), displacement, pqxx::sl::current())};
  PQXX_CHECK_EQUAL(std::size(rows), 3);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 1);
  PQXX_CHECK_EQUAL(rows[2][0].as<int>(), 3);
  PQXX_CHECK_EQUAL(displacement, 4);
  PQXX_CHECK_EQUAL(
    adopted.pos(), -1, "End-of-result set pos() on adopted cur");
  PQXX_CHECK_EQUAL(adopted.endpos(), -1, "endpos() set too early");

  rows = adopted.fetch(
    pqxx::cursor_base::backward_all(), displacement, pqxx::sl::current());
  PQXX_CHECK_EQUAL(std::size(rows), 3);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 3);
  PQXX_CHECK_EQUAL(rows[2][0].as<int>(), 1);
  PQXX_CHECK_EQUAL(displacement, -4);
  PQXX_CHECK_EQUAL(adopted.pos(), 0, "Failed to recognize starting position");
  PQXX_CHECK_EQUAL(adopted.endpos(), -1, "endpos() set too early");

  auto offset{adopted.move(pqxx::cursor_base::all(), pqxx::sl::current())};
  PQXX_CHECK_EQUAL(offset, 3);
  PQXX_CHECK_EQUAL(adopted.pos(), 4);
  PQXX_CHECK_EQUAL(adopted.endpos(), 4);

  // Owned adopted cursors are cleaned up on destruction.
  pqxx::connection conn2;
  pqxx::work tx2(conn2, "tx2");
  tx2
    .exec(
      "DECLARE adopted2 CURSOR FOR "
      "SELECT generate_series(1, 3)")
    .no_rows();
  {
    pqxx::internal::sql_cursor(tx2, "adopted2", pqxx::cursor_base::owned);
  }
  // Modern backends: accessing the cursor now is an error, as you'd expect.
  PQXX_CHECK_THROWS(
    tx2.exec("FETCH 1 IN adopted2"), pqxx::sql_error,
    "Owned adopted cursor not cleaned up");

  tx2.abort();

  pqxx::work tx3(conn2, "tx3");
  tx3.exec(
    "DECLARE adopted3 CURSOR FOR "
    "SELECT generate_series(1, 3)");
  {
    pqxx::internal::sql_cursor(tx3, "adopted3", pqxx::cursor_base::loose);
  }
  tx3.exec("MOVE 1 IN adopted3");
}

void test_hold_cursor(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  // "With hold" cursor is kept after commit.
  pqxx::internal::sql_cursor with_hold(
    tx, "SELECT generate_series(1, 3)", "hold_cursor",
    pqxx::cursor_base::forward_only, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, true);
  tx.commit();
  pqxx::work tx2(cx, "tx2");
  auto rows{with_hold.fetch(1, pqxx::sl::current())};
  PQXX_CHECK_EQUAL(
    std::size(rows), 1, "Did not get 1 row from with-hold cursor");

  // Cursor without hold is closed on commit.
  pqxx::internal::sql_cursor no_hold(
    tx2, "SELECT generate_series(1, 3)", "no_hold_cursor",
    pqxx::cursor_base::forward_only, pqxx::cursor_base::read_only,
    pqxx::cursor_base::owned, false);
  tx2.commit();
  pqxx::work const tx3(cx, "tx3");
  PQXX_CHECK_THROWS(
    no_hold.fetch(1, pqxx::sl::current()), pqxx::sql_error,
    "Cursor not closed on commit");
}


PQXX_REGISTER_TEST(test_forward_sql_cursor);
PQXX_REGISTER_TEST(test_scroll_sql_cursor);
PQXX_REGISTER_TEST(test_adopted_sql_cursor);
PQXX_REGISTER_TEST(test_hold_cursor);
} // namespace
