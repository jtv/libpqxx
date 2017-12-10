/* Reference documentation for libpqxx.
 *
 * Documentation is generated from header files.  This header is here only to
 * provide parts of that documentation.  There is no need to include it from
 * client code.
 *
 * Copyright 2001-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */

/** @mainpage
 * @author Jeroen T. Vermeulen
 *
 * Welcome to libpqxx, the C++ API to the PostgreSQL database management system.
 *
 * Compiling this package requires PostgreSQL to be installed -- including the
 * C headers for client development.  The library builds on top of PostgreSQL's
 * standard C API, libpq.  The libpq headers are not needed to compile client
 * programs, however.
 *
 * For a quick introduction to installing and using libpqxx, see the README.md
 * file; a more extensive tutorial is available in doc/html/Tutorial/index.html.
 *
 * The latest information can be found at http://pqxx.org/
 *
 * Some links that should help you find your bearings:
 * \li \ref gettingstarted
 * \li \ref threading
 * \li \ref connection
 * \li \ref transaction
 * \li \ref performance
 * \li \ref transactor
 *
 * @see http://pqxx.org/
 */

/** @page gettingstarted Getting started
 * The most basic three types in libpqxx are the connection (which inherits its
 * API from pqxx::connection_base and its setup behaviour from
 * pqxx::connectionpolicy), the transaction (derived from
 * pqxx::transaction_base), and the result (pqxx::result).
 *
 * They fit together as follows:
 * \li You connect to the database by creating a
 * connection object (see \ref connection).  The connection type you'll usually
 * want is pqxx::connection.
 * \li You create a transaction object (see \ref transaction) operating on that
 * connection.  You'll usually want the pqxx::work variety.  If you don't want
 * transactional behaviour, use pqxx::nontransaction.  Once you're done you call
 * the transaction's @c commit function to make its work final.  If you don't
 * call this, the work will be rolled back when the transaction object is
 * destroyed.
 * \li Until then, use the transaction's @c exec() functions to execute
 * queries, which you pass in as simple strings.
 * \li Most of the @c exec() functions return a pqxx::result object, which acts
 * as a standard container of rows.  Each row in itself acts as a container of
 * fields.  You can use array indexing and/or iterators to access either.
 * \li The field's data is stored as a text string.  You can read it as such
 * using its @c c_str() function, or convert it to other types using its @c as()
 * and @c to() member functions.  These are templated on the destination type:
 * @c myfield.as<int>(); or @c myfield.to(myint);
 * \li After you've closed the transaction, the connection is free to run a next
 * transaction.
 *
 * Here's a very basic example.  It connects to the default database (you'll
 * need to have one set up), queries it for a very simple result, converts it to
 * an @c int, and prints it out.  It also contains some basic error handling.
 *
 * @code
 * #include <iostream>
 * #include <pqxx/pqxx>
 *
 * int main()
 * {
 *   try
 *   {
 *     // Connect to the database.  In practice we may have to pass some
 *     // arguments to say where the database server is, and so on.
 *     pqxx::connection c;
 *
 *     // Start a transaction.  In libpqxx, you always work in one.
 *     pqxx::work w(c);
 *
 *     // work::exec1() executes a query returning a single row of data.
 *     // We'll just ask the database to return the number 1 to us.
 *     pqxx::row r = w.exec1("SELECT 1");
 *
 *     // Commit your transaction.  If an exception occurred before this
 *     // point, execution will have left the block, and the transaction will
 *     // have been destroyed along the way.  In that case, the failed
 *     // transaction would implicitly abort instead of getting to this point.
 *     w.commit();
 *
 *     // Look at the first and only field in the row, parse it as an integer,
 *     // and print it.
 *     std::cout << r[0].as<int>() << std::endl;
 *   }
 *   catch (const std::exception &e)
 *   {
 *     std::cerr << e.what() << std::endl;
 *     return 1;
 *   }
 * }
 * @endcode
 *
 * This prints the number 1.  Notice that you can keep the result object
 * around after the transaction (or even the connection) has been closed.
 *
 * Here's a slightly more complicated example.  It takes an argument from the
 * command line and retrieves a string with that value.  The interesting part is
 * that it uses the escaping-and-quoting function @c quote() to embed this
 * string value in SQL safely.  It also reads the result field's value as a
 * plain C-style string using its @c c_str() function.
 *
 * @code
 * #include <iostream>
 * #include <stdexcept>
 * #include <pqxx/pqxx>
 *
 * int main(int argc, char *argv[])
 * {
 *   try
 *   {
 *     if (!argv[1]) throw std::runtime_error("Give me a string!");
 *
 *     pqxx::connection c;
 *     pqxx::work w(c);
 *
 *     // work::exec() returns a full result set, which can consist of any
 *     // number of rows.
 *     pqxx::result r = w.exec("SELECT " + w.quote(argv[1]));
 *
 *     // End our transaction here.  We can still use the result afterwards.
 *     w.commit();
 *
 *     // Print the first field of the first row.  Read it as a C string,
 *     // just like std::string::c_str() does.
 *     std::cout << r[0][0].c_str() << std::endl;
 *   }
 *   catch (const std::exception &e)
 *   {
 *     std::cerr << e.what() << std::endl;
 *     return 1;
 *   }
 * }
 * @endcode
 *
 * You can find more about converting field values to native types, or
 * converting values to strings for use with libpqxx, under
 * \ref stringconversion.  More about getting to the rows and fields of a
 * result is under \ref accessingresults.
 *
 * If you want to handle exceptions thrown by libpqxx in more detail, for
 * example to print the SQL contents of a query that failed, see \ref exception.
 */

/** @page accessingresults Accessing results and result rows
 *
 * Let's say you have a result object.  For example, your program may have done:
 *
 * @code
 * pqxx::result r = w.exec("SELECT * FROM mytable");
 * @endcode
 *
 * Now how do you access the data inside @c r?
 *
 * The simplest way is array indexing.  A result acts as an array of rows,
 * and a row acts as an array of fields.
 *
 * @code
 * const int num_rows = r.size();
 * for (int rownum=0; rownum < num_rows; ++rownum)
 * {
 *   const pqxx::row row = r[rownum];
 *   const int num_cols = row.size();
 *   for (int colnum=0; colnum < num_cols; ++colnum)
 *   {
 *     const pqxx::field field = row[colnum];
 *     std::cout << field.c_str() << '\t';
 *   }
 *
 *   std::cout << std::endl;
 * }
 * @endcode
 *
 * But results and rows also define @c const_iterator types:
 *
 * @code
 * for (const auto &row: r)
 *  {
 *    for (const auto &field: row) std::cout << field.c_str() << '\t';
 *    std::cout << std::endl;
 *  }
 * @endcode
 *
 * They also have @c const_reverse_iterator types, which iterate backwards from
 * @c rbegin() to @c rend() exclusive.
 *
 * All these iterator types provide one extra bit of convenience that you won't
 * normally find in C++ iterators: referential transparency.  You don't need to
 * dereference them to get to the row or field they refer to.  That is, instead
 * of @c row->end() you can also choose to say @c row.end().  Similarly, you
 * may prefer @c field.c_str() over @c field->c_str().
 *
 * This becomes really helpful with the array-indexing operator.  With regular
 * C++ iterators you would need ugly expressions like @c (*row)[0] or
 * @c row->operator[](0).  With the iterator types defined by the result and
 * row classes you can simply say @c row[0].
 */

/** @page threading Thread safety
 *
 * This library does not contain any locking code to protect objects against
 * simultaneous modification in multi-threaded programs.  Therefore it is up
 * to you, the user of the library, to ensure that your threaded client
 * programs perform no conflicting operations concurrently.
 *
 * Most of the time this isn't hard.  Result sets are immutable, so you can
 * share them between threads without problem.  The main rule is:
 *
 * \li Treat a connection, together with any and all objects related to it, as
 * a "world" of its own.  You should generally make sure that the same "world"
 * is never accessed by another thread while you're doing anything non-const
 * in there.
 *
 * That means: don't issue a query on a transaction while you're also opening
 * a subtransaction, don't access a cursor while you may also be committing,
 * and so on.
 *
 * In particular, cursors are tricky.  It's easy to perform a non-const
 * operation without noticing.  So, if you're going to share cursors or
 * cursor-related objects between threads, lock very conservatively!
 *
 * Use @c pqxx::describe_thread_safety to find out at runtime what level of
 * thread safety is implemented in your build and version of libpqxx.  It
 * returns a pqxx::thread_safety_model describing what you can and cannot rely
 * on.  A command-line utility @c tools/pqxxthreadsafety prints out the same
 * information.
 */
