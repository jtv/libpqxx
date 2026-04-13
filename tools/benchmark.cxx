#include <libpq-fe.h>
#include <pqxx/pqxx>

#include <pqxx/internal/wait.hxx>

#include <format>
#include <iostream>

namespace
{
/// Fatal but well-handled error.
struct fail final : std::runtime_error
{
  fail(std::string const &whatarg) : std::runtime_error{whatarg} {}
};


/// Early successful exit.
struct early_exit final : std::exception
{};


struct options
{
  /// Number of rows to test.
  std::size_t size = 100u;

  /// Number of columns in result sets to test.
  /** Must be greater than zero, but smaller than 40.
   */
  std::size_t columns = 10u;

  /// Processing delay for each row of data, in microseconds.
  /** Use this to simulate how long it takes an application to process a row of
   * data received from the database.
   *
   * Honestly it's kind of pointless right now: a delay of just one microsecond
   * will dominate the benchmark and swamp out the speed differences.
   */
  unsigned delay = 0u;

  /// Database connection string.
  std::string connect = "";

  /// Encoding name.
  std::string encoding = "utf8";
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
      "{}-{} columns={} delay={}micros enc={} rows={}", base_name, scenario,
      columns, opts().delay, opts().encoding, opts().size);
  }

  /// Delay to simulate processing of a row's data in the application.
  void delay() const { pqxx::internal::wait_for(opts().delay); }

  /// Query and process `rows` rows of `columns` integers.
  /** The processing consists of adding up the unsigned integral values of each
   * field, pausing for the configured per-row delay.
   */
  template<std::size_t columns> std::size_t query_ints();

private:
  options m_opts;
};


template<typename BM>
concept benchmark_type =
  std::derived_from<BM, benchmark> and requires { BM::name(); };


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
  pq_result(options o) :
          benchmark{std::move(o)},
          m_cx(PQconnectdb(opts().connect.c_str()), [](PGconn *ptr) {
            PQfinish(ptr);
          })
  {
    if (m_cx == nullptr)
      throw std::bad_alloc{};
    check_conn();
    if (PQsetClientEncoding(m_cx.get(), opts().encoding.c_str()) != 0)
      throw std::runtime_error{
        std::format("Setting client encoding {} failed.", opts().encoding)};
  }

  static constexpr std::string_view name() noexcept { return "pq_result"; }

  template<std::size_t columns> std::size_t query_ints()
  {
    auto const rows{opts().size};
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
    std::size_t output{0};
    for (std::size_t row{0u}; row < actual_rows; ++row)
    {
      delay();
      for (std::size_t column{0u}; column < columns; ++column)
      {
        auto const field = PQgetvalue(
          res.get(), static_cast<int>(row), static_cast<int>(column));
        if (field == nullptr)
          throw std::runtime_error{"No value in field!"};

        output += parse_size_t(field);
      }
    }
    return output;
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

  static std::size_t parse_size_t(std::string_view field)
  {
    auto const start = std::data(field), end = start + std::size(field);
    std::size_t value{};
    auto const success = std::from_chars(start, end, value);
    if (success.ec != std::errc{})
      throw fail{std::format("Could not convert to number: '{}'", field)};
    if (success.ptr != end)
      throw fail{std::format("Could not parse whole field: '{}'", field)};
    return value;
  }
};


/// Benchmarks for libpqxx, with result objects.
class pqxx_result final : public benchmark
{
public:
  pqxx_result(options o) :
          benchmark{std::move(o)}, m_cx{opts().connect.c_str()}
  {
    m_cx.set_client_encoding(opts().encoding);
  }

  static constexpr std::string_view name() noexcept { return "pqxx_result"; }

  template<std::size_t columns> std::size_t query_ints()
  {
    auto const rows{opts().size};
    if (std::cmp_greater(rows, (std::numeric_limits<int>::max)()))
      throw fail{std::format(
        "Number of rows ({}) is greater than can be indexed.", rows)};
    pqxx::nontransaction tx{m_cx};
    auto const res = tx.exec(compose_ints_query(rows, columns));
    res.expect_columns(columns);
    std::size_t output{0};
    for (auto const row : res)
    {
      delay();
      for (auto const field : row) output += field.as<std::size_t>();
    }
    return output;
  }

private:
  pqxx::connection m_cx;
};


/// Benchmarks for libpqxx, with streaming.
class pqxx_stream final : public benchmark
{
public:
  pqxx_stream(options o) :
          benchmark{std::move(o)}, m_cx{opts().connect.c_str()}
  {
    m_cx.set_client_encoding(opts().encoding);
  }

  static constexpr std::string_view name() noexcept { return "pqxx_stream"; }

  template<std::size_t columns> std::size_t query_ints()
  {
    auto const rows{opts().size};
    pqxx::nontransaction tx{m_cx};
    auto const query = compose_ints_query(rows, columns);
    std::size_t actual_rows = 0u;
    std::size_t output = 0u;

    auto stream{[&]<std::size_t... Is>(std::index_sequence<Is...>) {
      return tx
        .stream<typename repeat_n<std::size_t, columns>::template type<Is>...>(
          query);
    }(std::make_index_sequence<columns>{})};

    for (auto row : stream)
    {
      delay();

      // This is also horrible code because it's something you never need to
      // do in the real world, and so nothing is designed for it.  There are
      // probably nicer ways to do it, but it'll do for now.
      // C++26: Consider using "template for".
      output += process_field<columns, 0>(row);
      output += process_field<columns, 1>(row);
      output += process_field<columns, 2>(row);
      output += process_field<columns, 3>(row);
      output += process_field<columns, 4>(row);
      output += process_field<columns, 5>(row);
      output += process_field<columns, 6>(row);
      output += process_field<columns, 7>(row);
      output += process_field<columns, 8>(row);
      output += process_field<columns, 9>(row);
      output += process_field<columns, 10>(row);
      output += process_field<columns, 11>(row);
      output += process_field<columns, 12>(row);
      output += process_field<columns, 13>(row);
      output += process_field<columns, 14>(row);
      output += process_field<columns, 15>(row);
      output += process_field<columns, 16>(row);
      output += process_field<columns, 17>(row);
      output += process_field<columns, 18>(row);
      output += process_field<columns, 19>(row);
      output += process_field<columns, 20>(row);
      output += process_field<columns, 21>(row);
      output += process_field<columns, 22>(row);
      output += process_field<columns, 23>(row);
      output += process_field<columns, 24>(row);
      output += process_field<columns, 25>(row);
      output += process_field<columns, 26>(row);
      output += process_field<columns, 27>(row);
      output += process_field<columns, 28>(row);
      output += process_field<columns, 29>(row);
      output += process_field<columns, 30>(row);
      output += process_field<columns, 31>(row);
      output += process_field<columns, 32>(row);
      output += process_field<columns, 33>(row);
      output += process_field<columns, 34>(row);
      output += process_field<columns, 35>(row);
      output += process_field<columns, 36>(row);
      output += process_field<columns, 37>(row);
      output += process_field<columns, 38>(row);
      output += process_field<columns, 39>(row);
      static_assert(columns <= 40);

      ++actual_rows;
    }

    if (actual_rows != rows)
      throw std::runtime_error{
        std::format("Expected {} row(s), got {}.", rows, actual_rows)};

    return output;
  }

private:
  /// Helper for constructing a parameter pack of `n` `T`s.
  template<typename T, std::size_t N> struct repeat_n
  {
    template<std::size_t> using type = T;
  };

  /// Return a field's numeric value, or 0 if it's not applicable.
  template<std::size_t columns, std::size_t field>
  static std::size_t process_field(auto const &row)
  {
    if constexpr (columns > field)
      return std::get<field>(row);
    else
      return 0u;
  }

  pqxx::connection m_cx;
};


template<typename FUNC>
concept benchmark_func = std::invocable<FUNC>;


template<benchmark_func FUNC>
inline std::size_t time_bench(std::string_view name, FUNC const &code)
{
  using timer = std::chrono::steady_clock;

  std::cerr << name << ": ";

  auto const start{timer::now()};
  auto const output{code()};
  auto const finish{timer::now()};
  auto const seconds{
    std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(
      finish - start)};

  std::cerr.setf(std::ios::fixed);
  std::cerr << seconds.count() << "s\n";

  return output;
}


template<benchmark_type Benchmark, std::size_t Columns>
std::size_t measure(options const &opts)
{
  Benchmark bench{opts};
  return time_bench(
    bench.template describe<Columns>(Benchmark::name(), "ints"),
    [&bench] { return bench.template query_ints<Columns>(); });
}


template<std::integral T = std::size_t> constexpr T ipow(T base, T exp)
{
  if (base == 0)
    return 0;

  auto const limit{((std::numeric_limits<T>::max)() - 1) / base};
  T result{1};
  for (std::size_t i{0}; i < exp; ++i)
  {
    if (result > limit)
      throw fail{std::format("Number too large: {}^{}", base, exp)};
    result *= base;
  }
  return result;
}


/// Parse a human-provided, integral number.
/** Supports a bit of extra syntax: you can write numbers like 10^3 for 1,000.
 */
template<std::integral T = std::size_t>
T parse_human_number(std::string_view text)
{
  auto const caret = text.find('^');
  if (caret == std::string_view::npos)
  {
    // Just a regular number.
    return pqxx::from_string<T>(text);
  }
  else
  {
    // Caret notation.
    T base = pqxx::from_string<T>(text.substr(0, caret)),
      exp = pqxx::from_string<T>(text.substr(caret + 1));
    if (std::cmp_equal(base, 0) and std::cmp_equal(exp, 0))
      throw fail{"The meaning of 0^0 is mathematically ambiguous."};
    return ipow(base, exp);
  }
}


constexpr static auto help_output =
  R"xx(Query benchmark for libpqxx.

Times some simple queries using various libpqxx calls, as well as using raw
libpq calls.

Options:
  --columns <C> or -w <C>
      Query data <C> columns wide.
  --connect <C> or -c <C>
      Connect to database using connection string <C>.
  --delay <D> or -d <D>
      Pause for <D> microseconds for each row, to simulate processing.
  --encoding <E> or -e <E>
      Simulate processing using client encoding <E>, e.g. UTF-8 or SJIS or
      GB18030.  Encodings can differ in their performance characteristics.
  --help or -h
      Show this explanation, and exit.
  --size <R> or -s <R>
      Query <R> rows of data.

For the numeric arguments, you can pass either a number or a simple "x to the
power of y" formula, such as "10^3" for 1,000 or "2^8" for 256.
)xx";


[[noreturn]] void exit_with_help()
{
  std::cout << help_output;
  throw early_exit{};
}


/// Options that take an argument.
enum class arg_opts
{
  none,
  columns,
  connect,
  delay,
  encoding,
  size
};


options parse_opts(char *argv[])
{
  options opts;

  arg_opts want = arg_opts::none;
  for (std::size_t i{1}; argv[i]; ++i)
  {
    std::string_view const arg{argv[i]};
    if (want == arg_opts::none)
    {
      // We're expecting a fresh command-line option.
      // TODO: Support "--size=10" and "-s10" syntax.
      if ((arg == "--columns") or (arg == "-w"))
        want = arg_opts::columns;
      else if ((arg == "--connect") or (arg == "-c"))
        want = arg_opts::connect;
      else if ((arg == "--delay") or (arg == "-d"))
        want = arg_opts::delay;
      else if ((arg == "--encoding") or (arg == "-e"))
        want = arg_opts::encoding;
      else if ((arg == "--help") or (arg == "-h"))
        exit_with_help();
      else if ((arg == "--size") or (arg == "-s"))
        want = arg_opts::size;
      else
        throw fail{std::format("Unexpected argument: '{}'.", arg)};
    }
    else
    {
      // The previous option was one that requires an argument.  Parse `arg`
      // as such.
      switch (want)
      {
      case arg_opts::columns: opts.columns = parse_human_number(arg); break;
      case arg_opts::connect: opts.connect = arg; break;
      case arg_opts::delay:
        opts.delay = parse_human_number<decltype(options::delay)>(arg);
        break;
      case arg_opts::encoding: opts.encoding = arg; break;
      case arg_opts::size: opts.size = parse_human_number(arg); break;
      case arg_opts::none: PQXX_UNREACHABLE;
      }
      // In each of these cases, we no longer want the next command-line item
      // to be an argument to the option.
      want = arg_opts::none;
    }
  }

  if (want != arg_opts::none)
    throw fail{"Last option is missing an argument."};

  return opts;
}


/// Run benchmarks, compare sums as a correctness check.
template<std::size_t columns> void run_and_compare(options const &opts)
{
  auto const pq_result_value{measure<pq_result, columns>(opts)};
  auto const pqxx_result_value{measure<pqxx_result, columns>(opts)};
  auto const pqxx_stream_value{measure<pqxx_stream, columns>(opts)};

  if (
    (pqxx_result_value != pq_result_value) or
    (pqxx_stream_value != pq_result_value))
  {
    throw fail{std::format(
      "Inconsistent computation results: pq_result-int returned {}, "
      "pqxx_result-int {}, pqxx_stream-int {}.",
      pq_result_value, pqxx_result_value, pqxx_stream_value)};
  }
}
} // namespace


int main(int, char *argv[])
{
  try
  {
    options const opts{parse_opts(argv)};

    switch (opts.columns)
    {
    case 0: throw fail{"Results must contain at least one column."};
    case 1: run_and_compare<1>(opts); break;
    case 2: run_and_compare<2>(opts); break;
    case 3: run_and_compare<3>(opts); break;
    case 4: run_and_compare<4>(opts); break;
    case 5: run_and_compare<5>(opts); break;
    case 6: run_and_compare<6>(opts); break;
    case 7: run_and_compare<7>(opts); break;
    case 8: run_and_compare<8>(opts); break;
    case 9: run_and_compare<9>(opts); break;
    case 10: run_and_compare<10>(opts); break;
    case 11: run_and_compare<11>(opts); break;
    case 12: run_and_compare<12>(opts); break;
    case 13: run_and_compare<13>(opts); break;
    case 14: run_and_compare<14>(opts); break;
    case 15: run_and_compare<15>(opts); break;
    case 16: run_and_compare<16>(opts); break;
    case 17: run_and_compare<17>(opts); break;
    case 18: run_and_compare<18>(opts); break;
    case 19: run_and_compare<19>(opts); break;
    case 20: run_and_compare<20>(opts); break;
    case 21: run_and_compare<21>(opts); break;
    case 22: run_and_compare<22>(opts); break;
    case 23: run_and_compare<23>(opts); break;
    case 24: run_and_compare<24>(opts); break;
    case 25: run_and_compare<25>(opts); break;
    case 26: run_and_compare<26>(opts); break;
    case 27: run_and_compare<27>(opts); break;
    case 28: run_and_compare<28>(opts); break;
    case 29: run_and_compare<29>(opts); break;
    case 30: run_and_compare<30>(opts); break;
    case 31: run_and_compare<31>(opts); break;
    case 32: run_and_compare<32>(opts); break;
    case 33: run_and_compare<33>(opts); break;
    case 34: run_and_compare<34>(opts); break;
    case 35: run_and_compare<35>(opts); break;
    case 36: run_and_compare<36>(opts); break;
    case 37: run_and_compare<37>(opts); break;
    case 38: run_and_compare<38>(opts); break;
    case 39: run_and_compare<39>(opts); break;
    case 40: run_and_compare<39>(opts); break;
    default: throw fail{"Maximum supported number of columns is 40."};
    }
  }
  catch (early_exit const &)
  {
    return 0;
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
