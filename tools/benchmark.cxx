#include <postgresql/libpq-fe.h>
#include <pqxx/pqxx>

#include <format>
#include <iostream>

// XXX: Set client encoding!
namespace
{
/// Hollow base class for comparable benchmarks.
class benchmark
{
public:
  /// Derived-class constructors will connect to the database.
  /** The derived-class constructors take a connection string and a client
   * encoding string.
   */
  benchmark() = default;

  /// Query and process `rows` rows of `columns` integers.
  /** The processing consists of reading each field; parsing it to an `int`;
   * and writing it to `cout`.
   *
   * Each row ends in a newline character.  Each field is followed by a space.
   */
  template<std::size_t columns> void query_ints(std::size_t rows);

  /// Query and process `rows` rows of `columns` strings, of average `length`.
  /** The processing consists of reading each field, and printing it to
   * `cout`.
   */
  template<std::size_t columns>
  void query_strings(std::size_t rows, std::size_t length);
};


/// Generate SQL to query `rows` rows of `columns` integers each.
std::string compose_ints_query(std::size_t rows, std::size_t columns)
{
  std::string tail;
  for (std::size_t c{1u}; c < columns; ++c) tail += std::format(", n*{}", c);
  return std::format(
    "SELECT n{} FROM generate_series(1, {}) AS n", tail, rows);
}


/// Benchmarks for libpq, with result objects.
/** This one's a lot of work.  That's actually one of the main reasons for
 * libpqxx to exist in the first place.  I'm not even attempting a streaming
 * query, with all the encoding support, handling of quotes and escapes, etc.
 */
class pq_result final
{
public:
  pq_result(char const *connstr = "", char const *encoding = "utf8") :
          m_cx{PQconnectdb(connstr)}
  {
    if (m_cx == nullptr)
      throw std::bad_alloc{};
    check_conn();
    if (PQsetClientEncoding(m_cx, encoding) != 0)
      throw std::runtime_error{
        std::format("Setting client encoding {} failed.", encoding)};
  }

  ~pq_result() { PQfinish(m_cx); }

  /// This benchmark's name.
  static constexpr std::string_view name = "pq_result";

  template<std::size_t columns> void query_ints(std::size_t rows)
  {
    auto const conn = compose_ints_query(rows, columns);
    auto const res = PQexec(m_cx, conn.c_str());
    check_result(res);
    if (PQnfields(res) != columns)
      throw std::runtime_error{std::format(
        "Expected {} column(s), got {}.", columns, PQnfields(res))};
    std::size_t const actual_rows{static_cast<std::size_t>(PQntuples(res))};
    for (std::size_t row{0u}; row < actual_rows; ++row)
    {
      for (std::size_t column{0u}; column < columns; ++column)
      {
        auto const field =
          PQgetvalue(res, static_cast<int>(row), static_cast<int>(column));
        if (field == nullptr)
          throw std::runtime_error{"No value in field!"};
        std::stringstream parser;
        parser << field;
        int integer;
        parser >> integer;
        std::cout << integer << ' ';
      }
      std::cout << '\n';
    }
    PQclear(res);
  }

private:
  PGconn *m_cx;

  /// Throw exception if connection is in a bad state.
  void check_conn() const
  {
    if (m_cx == nullptr)
      throw std::runtime_error{"No connection."};
    if (PQstatus(m_cx) != CONNECTION_OK)
      throw std::runtime_error{PQerrorMessage(m_cx)};
  }

  /// Throw exception if `res` indicates a failed query.
  void check_result(PGresult *res) const
  {
    if (res == nullptr)
    {
      check_conn();
    }
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
      throw std::runtime_error{PQresultErrorMessage(res)};
  }
};


/// Benchmarks for libpqxx, with result objects.
class pqxx_result final
{
public:
  pqxx_result(char const *connstr = "", char const *encoding = "utf8") :
          m_cx{connstr}
  {
    m_cx.set_client_encoding(encoding);
  }

  /// This benchmark's name.
  static constexpr std::string_view name = "pq_result";

  template<std::size_t columns> void query_ints(std::size_t rows)
  {
    pqxx::nontransaction tx{m_cx};
    auto const res = tx.exec(compose_ints_query(rows, columns));
    res.expect_columns(columns);
    for (auto const row : res)
    {
      for (auto const field : row) std::cout << field.as<int>() << '\n';
      std::cout << '\n';
    }
  }

private:
  pqxx::connection m_cx;
};


/// Benchmarks for lipqxx, with streaming.
class pqxx_stream final
{
public:
  pqxx_stream(char const *connstr = "", char const *encoding = "utf8") :
          m_cx{connstr}
  {
    m_cx.set_client_encoding(encoding);
  }

  /// This benchmark's name.
  static constexpr std::string_view name = "pq_result";

  template<std::size_t columns> void query_ints(std::size_t rows)
  {
    pqxx::nontransaction tx{m_cx};
    auto const query = compose_ints_query(rows, columns);
    std::size_t actual_rows = 0u;

    // Call tx.stream<int, int, int...>(...).  In real-world code this is a
    // strange and unusual pattern.
    auto stream{[&]<std::size_t... Is>(std::index_sequence<Is...>) {
      return tx.stream<typename repeat_n<int, columns>::template type<Is>...>(
        query);
      // or: obj.bar<typename repeat_n<int, columns>::template type<Is>...>();
    }(std::make_index_sequence<columns>{})};

    for (auto row : stream)
    {
      // This is also horrible code because it's something you never need to do
      // in the real world, and so nothing is designed for it.  There are
      // probably nicer ways to do it, but it'll do for now.
      // C++26: Consider using "template for".
      if constexpr (columns > 0)
        std::cout << std::get<0>(row) << ' ';
      if constexpr (columns > 1)
        std::cout << std::get<1>(row) << ' ';
      if constexpr (columns > 2)
        std::cout << std::get<2>(row) << ' ';
      if constexpr (columns > 3)
        std::cout << std::get<3>(row) << ' ';
      if constexpr (columns > 4)
        std::cout << std::get<4>(row) << ' ';
      if constexpr (columns > 5)
        std::cout << std::get<5>(row) << ' ';
      if constexpr (columns > 6)
        std::cout << std::get<6>(row) << ' ';
      if constexpr (columns > 7)
        std::cout << std::get<7>(row) << ' ';
      if constexpr (columns > 8)
        std::cout << std::get<8>(row) << ' ';
      if constexpr (columns > 9)
        std::cout << std::get<9>(row) << ' ';
      if constexpr (columns > 10)
        std::cout << std::get<10>(row) << ' ';
      if constexpr (columns > 11)
        std::cout << std::get<11>(row) << ' ';
      if constexpr (columns > 12)
        std::cout << std::get<12>(row) << ' ';
      if constexpr (columns > 13)
        std::cout << std::get<13>(row) << ' ';
      if constexpr (columns > 14)
        std::cout << std::get<14>(row) << ' ';
      if constexpr (columns > 15)
        std::cout << std::get<15>(row) << ' ';
      if constexpr (columns > 16)
        std::cout << std::get<16>(row) << ' ';
      if constexpr (columns > 17)
        std::cout << std::get<17>(row) << ' ';
      if constexpr (columns > 18)
        std::cout << std::get<18>(row) << ' ';
      if constexpr (columns > 19)
        std::cout << std::get<19>(row) << ' ';
      if constexpr (columns > 20)
        std::cout << std::get<20>(row) << ' ';
      if constexpr (columns > 21)
        std::cout << std::get<21>(row) << ' ';
      if constexpr (columns > 20)
        std::cout << std::get<22>(row) << ' ';
      if constexpr (columns > 23)
        std::cout << std::get<23>(row) << ' ';
      if constexpr (columns > 24)
        std::cout << std::get<24>(row) << ' ';
      if constexpr (columns > 25)
        std::cout << std::get<25>(row) << ' ';
      if constexpr (columns > 26)
        std::cout << std::get<26>(row) << ' ';
      if constexpr (columns > 27)
        std::cout << std::get<27>(row) << ' ';
      if constexpr (columns > 28)
        std::cout << std::get<28>(row) << ' ';
      if constexpr (columns > 29)
        std::cout << std::get<29>(row) << ' ';
      if constexpr (columns > 30)
        std::cout << std::get<30>(row) << ' ';
      if constexpr (columns > 31)
        std::cout << std::get<31>(row) << ' ';
      if constexpr (columns > 32)
        std::cout << std::get<32>(row) << ' ';
      if constexpr (columns > 33)
        std::cout << std::get<33>(row) << ' ';
      if constexpr (columns > 34)
        std::cout << std::get<34>(row) << ' ';
      if constexpr (columns > 35)
        std::cout << std::get<35>(row) << ' ';
      if constexpr (columns > 36)
        std::cout << std::get<36>(row) << ' ';
      if constexpr (columns > 37)
        std::cout << std::get<37>(row) << ' ';
      if constexpr (columns > 38)
        std::cout << std::get<38>(row) << ' ';
      if constexpr (columns > 39)
        std::cout << std::get<39>(row) << ' ';
      static_assert(columns <= 40);

      std::cout << '\n';
      ++actual_rows;
    }

    if (actual_rows != rows)
      throw std::runtime_error{
        std::format("Expected {} row(s), got {}.", rows, actual_rows)};
  }

private:
  /// Helper for constructing a parameter pack of `n` `int`s.
  template<typename T, std::size_t N> struct repeat_n
  {
    template<std::size_t> using type = T;
  };

  pqxx::connection m_cx;
};


template<typename benchmark> void measure(char const encoding[])
{
  std::cout << "Starting benchmark " << benchmark::name << " (" << encoding
            << ")\n";
  benchmark bench{"", encoding};

  for (std::size_t rows = 1u; rows < 100'000'000; rows *= 10)
  {
    // XXX: Start timer.
    bench.template query_ints<1>(rows);
    // XXX: Report time.

    // XXX: Start timer.
    bench.template query_ints<4>(rows);
    // XXX: Report time.

    // XXX: Start timer.
    bench.template query_ints<16>(rows);
    // XXX: Report time.

    // XXX: Start timer.
    bench.template query_ints<32>(rows);
    // XXX: Report time.
  }
}
} // namespace


int main()
{
  std::vector<char const *> const encodings{"sqlascii", "utf8", "sjis"};
  for (auto const encoding : encodings)
  {
    measure<pq_result>(encoding);
    measure<pqxx_result>(encoding);
    measure<pqxx_stream>(encoding);
  }
}
