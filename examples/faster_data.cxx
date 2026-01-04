#include <iostream>

#include <pqxx/pqxx>

/** An example showing how to optimise data processing for your needs in
 * libpqxx.
 */

namespace
{
/// Some processing function that we want to execute on a bunch of records.
/** Each of the records will consist of a string and an integer.
 */
int process_row(std::string_view name, int number)
{
  return number - static_cast<int>(std::ssize(name));
}


/// Generate a query that produces `num_rows` rows of data.
/** Each row will consist of a string and an integer.
 */
std::string make_query(int num_rows)
{
  return std::format(
    "SELECT ('name' || n), n FROM generate_series(1, {}) n", num_rows);
}
} // namespace


int main()
{
  try
  {
    pqxx::connection cx;
    pqxx::transaction tx{cx};

    // Here's the basic way to query the database.  It loads all the result
    // data into our memory, and returns a pqxx::result.
    pqxx::result const r1 = tx.exec(make_query(100));

    // We can ask the result to check that it has exactly 100 rows.  If it does
    // not, this will throw an exception.
    r1.expect_rows(100);

    // Here's a very basic way to run `process_row` on each of the rows, and
    // add up all the numbers.  It converts each row's two fields to a
    // `std::string` and an `int`, respectively.
    int sum1a = 0;
    for (auto const row : r1)
      sum1a += process_row(row.at(0).as<std::string>(), row.at(1).as<int>());

    // But, that loop does unnecessary work.  It reads each string into a full
    // std::string object, which can involve memory allocations and
    // deallocations internally.
    //
    // To get rid of that waste, you can instead read each string into a
    // std::string_view.  This is a lighter-weight object that's merely a
    // reference to data elsewhere.  But be careful: that's only valid for as
    // long as r1 (or a copy of it) remains in memory.  Otherwise, the
    // underlying result data will be deallocated and your reference will refer
    // to invalid memory.
    //
    // As a shortcut, instead of `.as<std::string_view>()`, you can also use
    // the simpler `.view()` member function.
    int sum1b = 0;
    for (auto const row : r1)
      sum1b +=
        process_row(row.at(0).as<std::string_view>(), row.at(1).as<int>());

    // This produces the exact same results.
    assert(sum1b == sum1a);

    // If you prefer a declarative style with callbacks, you can also use the
    // result's `for_each()` function.
    //
    // What's really neat about this is that you don't need to specify how you
    // want to read the fields.  The `for_each()` function inspects your
    // callback to figure out what parameter types it takes, and converts each
    // row's fields to those respective types.
    //
    // It figures this out at compile time, so there's no run-time cost.
    int sum1c = 0;
    r1.for_each([&sum1c](std::string_view name, int number) {
      sum1c += process_row(name, number);
    });

    // This again produces the same results.
    assert(sum1c == sum1a);

    // But all these are just the first way of querying data.  It reads all the
    // result data from the database server and returns an object representing
    // all that data.
    //
    // There is another way: _streaming._  This does not work for all queries;
    // it does not accept parameters, for instance.  Due to some constant
    // overhead it's also likely to be a bit _slower_ for small result sets.
    // But it gets much faster for larger result sets.  The actual numbers
    // depend on your individual use-case, so when performance is crucial,
    // measure what works best for you.
    //
    // Why does streaming tend to be faster?  There are several reasons:
    // 1. You can start processing the first rows before the query even
    // finishes.
    // 2. It bypasses calls to the underlying C library, libpq.
    // 3. Fewer memory allocations and deallocations are needed.
    // 4. Encoding support in libpqxx has very little overhead.
    //
    // When streaming a query, you specify as what types you want to read the
    // respective columns, as template arguments.
    int sum2a = 0;
    // This executes the query with a large number of result rows, and iterates
    // over each row as it streams in.
    for (auto const [name, number] :
         tx.stream<std::string_view, int>(make_query(10'000)))
      sum2a += process_row(name, number);

    // Streaming, too, has a variant where you pass a callback.
    int sum2b = 0;
    tx.for_stream(
      make_query(10'000), [&sum2b](std::string_view name, int number) {
        sum2b += process_row(name, number);
      });

    assert(sum2b == sum2a);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
