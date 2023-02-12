/* Code for parts of pqxx::stream_query.
 *
 * These definitions needs to be in a separate file in order to iron out
 * circular dependencies between headers.
 */
#if !defined(PQXX_H_STREAM_QUERY_IMPL)
#  define PQXX_H_STREAM_QUERY_IMPL

namespace pqxx
{
template<typename... TYPE>
inline stream_query<TYPE...>::stream_query(
  transaction_base &tx, std::string_view query) :
        transaction_focus{tx, "stream_query"}, m_char_finder{get_finder(tx)}
{
  tx.exec0(internal::concat("COPY (", query, ") TO STDOUT"));
  register_me();
}


template<typename... TYPE>
inline pqxx::internal::char_finder_func *
stream_query<TYPE...>::get_finder(transaction_base const &tx)
{
  auto const group{pqxx::internal::enc_group(tx.conn().encoding_id())};
  return pqxx::internal::get_char_finder<'\t', '\\'>(group);
}


// C++20: Replace with generator?
/// Input iterator for stream_query.
/** Just barely enough to support range-based "for" loops on stream_from.
 * Don't assume that any of the usual behaviour works beyond that.
 */
template<typename... TYPE> class stream_query_input_iterator
{
  using stream_t = stream_query<TYPE...>;

public:
  using value_type = std::tuple<TYPE...>;

  /// Construct an "end" iterator.
  stream_query_input_iterator() = default;

  explicit stream_query_input_iterator(stream_t &home) : m_home(&home)
  {
    advance();
  }
  stream_query_input_iterator(stream_query_input_iterator const &) = default;

  stream_query_input_iterator &operator++() &
  {
    advance();
    return *this;
  }

  value_type operator*()
  {
    return m_home->parse_line();
  }

  /// Are we at the end?
  bool operator==(stream_query_end_iterator) const noexcept
  {
    return done();
  }
  /// Comparison only works for comparing to end().
  bool operator!=(stream_query_end_iterator) const noexcept
  {
    return not done();
  }

private:
  bool done() const noexcept { return (m_home == nullptr) or m_home->done(); }

  void advance() &
  {
    assert(not done());
    m_home->read_line();
  }

  stream_t *m_home = nullptr;
};


template<typename... TYPE> inline bool
operator==(stream_query_end_iterator, stream_query_input_iterator<TYPE...> const &i)
{
  return i.done();
}


template<typename... TYPE> inline bool
operator!=(stream_query_end_iterator, stream_query_input_iterator<TYPE...> const &i)
{
  return not i.done();
}


template<typename... TYPE> inline auto stream_query<TYPE...>::begin() &
{
  return stream_query_input_iterator<TYPE...>{*this};
}


template<typename... TYPE> inline auto stream_query<TYPE...>::read_line() &
{
  assert(not done());

  internal::gate::connection_stream_from gate{m_trans.conn()};
  try
  {
    auto raw_line{gate.read_copy_line()};
    m_line = std::move(raw_line.first);
    m_line_size = raw_line.second;
    // Check for completion.
  }
  catch (std::exception const &)
  {
    close();
    throw;
  }
  if (not m_line) close();
  if (
    not done() and
    (m_line_size >= ((std::numeric_limits<decltype(m_line_size)>::max)() / 2))
  )
    throw range_error{"Stream produced a ridiculously long line."};
}


template<typename... TYPE> inline void stream_query<TYPE...>::complete()
{
  if (done())
    return;
  try
  {
    // Flush any remaining lines - libpq will automatically close the stream
    // when it hits the end.
    internal::gate::connection_stream_from gate{m_trans.conn()};
    while (not done()) gate.read_copy_line();
  }
  catch (broken_connection const &)
  {
    close();
    throw;
  }
  catch (std::exception const &e)
  {
    reg_pending_error(e.what());
  }
  close();
}
} // namespace pqxx
#endif
