#include <libpq-fe.h>
#include <pqxx/pqxx>

#include <pqxx/internal/wait.hxx>

#include <format>
#include <iostream>

// TODO: Is type conversion actually relevant to the benchmark?
// TODO: Configurable connection string.
// TODO: --help/-h option.
// TODO: More flexible syntax for specifying numbers of rows.
// TODO: Support options like --size=10 or -s10.
namespace
{
/// Fatal but well-handled error.
struct fail final : std::runtime_error
{
  fail(std::string const &whatarg) : std::runtime_error{whatarg} {}
};


struct options
{
  /// Number of rows to test, as an order of magnitude.
  /** The benchmark will try querying 1 row if this is zero, or 10 rows if it
   * is set to 1, or 100 rows if it's 2, and so on.
   */
  std::size_t size = 100u;

  /// Processing delay for each row of data, in microseconds.
  /** Use this to simulate how long it takes an application to process a row of
   * data received from the database.
   */
  unsigned delay = 10u;

  /// Encoding name.
  std::string encoding = "utf8";

  /// Print output?
  /** This adds significant overhead to the processing of each item of data.
   * Used for debugging, but also to stop the compiler from noticing that we
   * don't do anything productive with the data and eliminating it altogether.
   */
  bool print = false;
};


/// Hollow base class for comparable benchmarks.
class benchmark
{
public:
  /// Derived-class constructors will connect to the database.
  /** The derived-class constructors take a connection string and a client
   * encoding string.
   */
  benchmark() = delete;
  explicit benchmark(options opts) : m_opts{std::move(opts)} {}

  benchmark(benchmark const &) = default;
  benchmark(benchmark &&) = default;
  ~benchmark() = default;

  benchmark &operator=(benchmark const &) = default;
  benchmark &operator=(benchmark &&) = default;

  /// The `options` governing this benchmark.
  options const &opts() const noexcept { return m_opts; }

  /// This benchmark's base name, e.g. "pqxx-stream".
  static constexpr std::string_view name() noexcept { return "UNNAMED!"; }

  /// Generate a human-readable brief description for this benchmark.
  template<std::size_t columns>
  std::string
  describe(std::string_view base_name, std::string_view scenario) const
  {
    return std::format(
      "{} columns=1 delay={}ms enc={} rows={}", base_name, scenario,
      opts().delay, opts().encoding, opts().size);
  }

  /// Delay to simulate processing of a row's data in the application.
  void delay() const { pqxx::internal::wait_for(opts().delay); }

  /// Query and process `rows` rows of `columns` integers.
  /** The processing consists of reading each field; parsing it to a `size_t`,
   * pausing for the configured delay and, if `print` is true, writing the data
   * to `cout`.
   *
   * Each row ends in a newline character.  Each field is followed by a space.
   */
  template<std::size_t columns> void query_ints();

private:
  options m_opts;
};


template<typename BM>
concept benchmark_type = std::derived_from<BM, benchmark>;


/// Generate SQL to query `rows` rows of `columns` integers each.
std::string compose_ints_query(std::size_t rows, std::size_t columns)
{
  std::string tail;
  for (std::size_t c{1u}; c < columns; ++c)
    tail += std::format(", n*{}", c + 1);
  return std::format(
    "SELECT n{} FROM generate_series(1, {}) AS n", tail, rows);
}


/// Benchmarks for libpq, with result objects.
/** This one's a lot of work.  That's actually one of the main reasons for
 * libpqxx to exist in the first place.  I'm not even attempting a streaming
 * query, with all the encoding support, handling of quotes and escapes, etc.
 */
class pq_result final : public benchmark
{
public:
  pq_result(
    options opts, char const *connstr = "", char const *encoding = "utf8") :
          benchmark{opts},
          m_cx(PQconnectdb(connstr), [](PGconn *ptr) { PQfinish(ptr); })
  {
    if (m_cx == nullptr)
      throw std::bad_alloc{};
    check_conn();
    if (PQsetClientEncoding(m_cx.get(), encoding) != 0)
      throw std::runtime_error{
        std::format("Setting client encoding {} failed.", encoding)};
  }

  static constexpr std::string_view name() noexcept { return "pq_result"; }

  template<std::size_t columns> void query_ints()
  {
    auto const rows{opts().size};
    bool const print{opts().print};
    if (std::cmp_greater(rows, (std::numeric_limits<int>::max)()))
      throw fail{std::format(
        "Number of rows ({}) is greater than can be indexed.", rows)};
    auto const conn = compose_ints_query(rows, columns);
    std::unique_ptr<PGresult, decltype(&PQclear)> res{
      PQexec(m_cx.get(), conn.c_str()), &PQclear};
    check_result(res.get());
    if (PQnfields(res.get()) != columns)
      throw std::runtime_error{std::format(
        "Expected {} column(s), got {}.", columns, PQnfields(res.get()))};
    std::size_t const actual_rows{
      static_cast<std::size_t>(PQntuples(res.get()))};
    for (std::size_t row{0u}; row < actual_rows; ++row)
    {
      delay();
      for (std::size_t column{0u}; column < columns; ++column)
      {
        auto const field = PQgetvalue(
          res.get(), static_cast<int>(row), static_cast<int>(column));
        if (field == nullptr)
          throw std::runtime_error{"No value in field!"};

        std::size_t value{}, len{std::strlen(field)};
        auto const success{std::from_chars(field, field + len, value)};
        if (success.ec != std::errc{})
          throw fail{std::format("Could not convert number: {}", field)};
        if (success.ptr != (field + len))
          throw fail{std::format("Could not parse whole field: '{}'", field)};

        if (print)
        {
          if (column > 0u)
            std::cout << ' ';
          std::cout << value;
        }
      }
      if (print)
        std::cout << '\n';
    }
  }

private:
  std::unique_ptr<PGconn, void (*)(PGconn *)> m_cx;

  /// Throw exception if connection is in a bad state.
  void check_conn() const
  {
    if (m_cx == nullptr)
      throw std::runtime_error{"No connection."};
    if (PQstatus(m_cx.get()) != CONNECTION_OK)
      throw std::runtime_error{PQerrorMessage(m_cx.get())};
  }

  /// Throw exception if `res` indicates a failed query.
  void check_result(PGresult *res) const
  {
    if (res == nullptr) [[unlikely]]
    {
      check_conn();
      throw std::bad_alloc{};
    }
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
      throw std::runtime_error{PQresultErrorMessage(res)};
  }
};


/// Benchmarks for libpqxx, with result objects.
class pqxx_result final : public benchmark
{
public:
  pqxx_result(
    options opts, char const *connstr = "", char const *encoding = "utf8") :
          benchmark{std::move(opts)}, m_cx{connstr}
  {
    m_cx.set_client_encoding(encoding);
  }

  static constexpr std::string_view name() noexcept { return "pqxx_result"; }

  template<std::size_t columns> void query_ints()
  {
    auto const rows{opts().size};
    bool const print{opts().print};
    if (std::cmp_greater(rows, (std::numeric_limits<int>::max)()))
      throw fail{std::format(
        "Number of rows ({}) is greater than can be indexed.", rows)};
    pqxx::nontransaction tx{m_cx};
    auto const res = tx.exec(compose_ints_query(rows, columns));
    res.expect_columns(columns);
    for (auto const row : res)
    {
      delay();
      bool first{true};
      for (auto const field : row)
      {
        auto const value{field.as<std::size_t>()};
        if (print)
        {
          if (first)
            first = false;
          else
            std::cout << ' ';
          std::cout << value;
        }
      }
      if (print)
        std::cout << '\n';
    }
  }

private:
  pqxx::connection m_cx;
};


/// Benchmarks for libpqxx, with streaming.
class pqxx_stream final : public benchmark
{
public:
  pqxx_stream(
    options opts, char const *connstr = "", char const *encoding = "utf8") :
          benchmark{std::move(opts)}, m_cx{connstr}
  {
    m_cx.set_client_encoding(encoding);
  }

  static constexpr std::string_view name() noexcept { return "pqxx_stream"; }

  template<std::size_t columns> void query_ints()
  {
    auto const rows{opts().size};
    bool const print{opts().print};
    pqxx::nontransaction tx{m_cx};
    auto const query = compose_ints_query(rows, columns);
    std::size_t actual_rows = 0u;

    // Call tx.stream<size_t, size_t, size_t...>(...).  In real-world code this
    // is a strange and unusual pattern.
    auto stream{[&]<std::size_t... Is>(std::index_sequence<Is...>) {
      return tx
        .stream<typename repeat_n<std::size_t, columns>::template type<Is>...>(
          query);
      // or: obj.bar<typename repeat_n<std::size_t, columns>::template
      // type<Is>...>();
    }(std::make_index_sequence<columns>{})};

    for (auto row : stream)
    {
      delay();
      // XXX: Ensure the conversions still happen, even when not printing.
      if (print)
      {
        // This is also horrible code because it's something you never need to
        // do in the real world, and so nothing is designed for it.  There are
        // probably nicer ways to do it, but it'll do for now.
        // C++26: Consider using "template for".
        print_field<columns, 0>(row);
        print_field<columns, 1>(row);
        print_field<columns, 2>(row);
        print_field<columns, 3>(row);
        print_field<columns, 4>(row);
        print_field<columns, 5>(row);
        print_field<columns, 6>(row);
        print_field<columns, 7>(row);
        print_field<columns, 8>(row);
        print_field<columns, 9>(row);
        print_field<columns, 10>(row);
        print_field<columns, 11>(row);
        print_field<columns, 12>(row);
        print_field<columns, 13>(row);
        print_field<columns, 14>(row);
        print_field<columns, 15>(row);
        print_field<columns, 16>(row);
        print_field<columns, 17>(row);
        print_field<columns, 18>(row);
        print_field<columns, 19>(row);
        print_field<columns, 20>(row);
        print_field<columns, 21>(row);
        print_field<columns, 22>(row);
        print_field<columns, 23>(row);
        print_field<columns, 24>(row);
        print_field<columns, 25>(row);
        print_field<columns, 26>(row);
        print_field<columns, 27>(row);
        print_field<columns, 28>(row);
        print_field<columns, 29>(row);
        print_field<columns, 30>(row);
        print_field<columns, 31>(row);
        print_field<columns, 32>(row);
        print_field<columns, 33>(row);
        print_field<columns, 34>(row);
        print_field<columns, 35>(row);
        print_field<columns, 36>(row);
        print_field<columns, 37>(row);
        print_field<columns, 38>(row);
        print_field<columns, 39>(row);
        static_assert(columns <= 40);
        std::cout << '\n';
      }

      ++actual_rows;
    }

    if (actual_rows != rows)
      throw std::runtime_error{
        std::format("Expected {} row(s), got {}.", rows, actual_rows)};
  }

private:
  /// Helper for constructing a parameter pack of `n` `T`s.
  template<typename T, std::size_t N> struct repeat_n
  {
    template<std::size_t> using type = T;
  };

  /// Print a field value, followed by a space.
  template<std::size_t columns, std::size_t field>
  static void print_field(auto const &row)
  {
    if constexpr (columns > field)
    {
      if constexpr (field > 0)
        std::cout << ' ';
      std::cout << std::get<field>(row);
    }
  }

  pqxx::connection m_cx;
};


template<typename FUNC>
concept benchmark_func = std::invocable<FUNC>;


template<benchmark_func FUNC>
inline void time_bench(std::string_view name, FUNC const &code)
{
  using timer = std::chrono::steady_clock;

  std::cerr << name << ": ";

  auto const start{timer::now()};
  code();
  auto const finish{timer::now()};
  auto const seconds{
    std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(
      finish - start)};

  std::cerr.setf(std::ios::fixed);
  std::cerr << seconds.count() << "s\n";
}


template<benchmark_type Benchmark> void measure(options const &opts)
{
  Benchmark bench{opts, ""};

  time_bench(bench.template describe<1>(Benchmark::name(), "ints"), [&bench] {
    bench.template query_ints<1>();
  });
  time_bench(bench.template describe<4>(Benchmark::name(), "ints"), [&bench] {
    bench.template query_ints<4>();
  });
  time_bench(bench.template describe<16>(Benchmark::name(), "ints"), [&bench] {
    bench.template query_ints<16>();
  });
  time_bench(bench.template describe<32>(Benchmark::name(), "ints"), [&bench] {
    bench.template query_ints<32>();
  });
}


constexpr std::size_t ipow(std::size_t base, std::size_t exp)
{
  auto const limit{((std::numeric_limits<std::size_t>::max)() - 1) / base};
  std::size_t result{1};
  for (std::size_t i{0}; i < exp; ++i)
  {
    if (result > limit)
      throw fail{std::format("Number too large: {}**{}", base, exp)};
    result *= base;
  }
  return result;
}


options parse_opts(char *argv[])
{
  options opts;

  bool want_delay{false}, want_encoding{false}, want_size{false};

  for (std::size_t i{1}; argv[i]; ++i)
  {
    std::string_view const arg{argv[i]};
    assert((int(want_size) + int(want_encoding)) < 2);
    if (want_delay)
    {
      opts.delay = pqxx::from_string<decltype(opts.delay)>(arg);
      want_delay = false;
    }
    else if (want_encoding)
    {
      opts.encoding = arg;
      want_encoding = false;
    }
    else if (want_size)
    {
      opts.size = ipow(10, pqxx::from_string<std::size_t>(arg));
      want_size = false;
    }
    else if ((arg == "--delay") or (arg == "-d"))
    {
      want_delay = true;
    }
    else if ((arg == "--encoding") or (arg == "-e"))
    {
      want_encoding = true;
    }
    else if ((arg == "--print") or (arg == "-p"))
    {
      opts.print = true;
    }
    else if ((arg == "--size") or (arg == "-s"))
    {
      want_size = true;
    }
    else
    {
      throw fail{std::format("Unexpected argument: '{}'.", arg)};
    }
  }

  if (want_size)
    throw fail{"Missing argument to --size."};
  if (want_encoding)
    throw fail{"Missing argument to --encoding."};
  return opts;
}
} // namespace


int main(int, char *argv[])
{
  try
  {
    options const opts{parse_opts(argv)};

    measure<pq_result>(opts);
    measure<pqxx_result>(opts);
    measure<pqxx_stream>(opts);
  }
  catch (fail const &err)
  {
    std::cerr << err.what() << '\n';
    return 1;
  }
  catch (std::exception const &err)
  {
    std::cerr << err.what() << '\n';
    return 2;
  }
  return 0;
}
