/* Code for parts of pqxx::stream_query.
 *
 * These definitions needs to be in a separate file in order to iron out
 * circular dependencies between headers.
 */
#if !defined(PQXX_H_STREAM_QUERY_IMPL)
#define PQXX_H_STREAM_QUERY_IMPL

namespace pqxx
{
template<typename... TYPE> inline
stream_query<TYPE...>::stream_query(
  transaction_base &tx, std::string_view query
) : transaction_focus{tx, s_class_name}, m_char_finder{get_finder(tx)}
{
  tx.exec0(internal::concat("COPY (", query, ") TO STDOUT"));
  register_me();
}


template<typename... TYPE> inline
pqxx::internal::char_finder_func *
stream_query<TYPE...>::get_finder(transaction_base const &tx)
{
  auto const group{pqxx::internal::enc_group(tx.conn().encoding_id())};
  return pqxx::internal::get_char_finder<'\t', '\\'>(group);
}


template<typename... TYPE> inline auto stream_query<TYPE...>::get_raw_line() &
{
  assert(not done());
  internal::gate::connection_stream_from gate{m_trans.conn()};
  try
  {
    raw_line line{gate.read_copy_line()};
    if (line.first.get() == nullptr)
      close();
    return line;
  }
  catch (std::exception const &)
  {
    close();
    throw;
  }
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

  stream_query_input_iterator &operator++()
  {
    advance();
    return *this;
  }

  value_type const &operator*() const { return m_value; }

  /// Comparison only works for comparing to end().
  bool operator==(stream_query_input_iterator const &rhs) const
  {
    return m_home == rhs.m_home;
  }
  /// Comparison only works for comparing to end().
  bool operator!=(stream_query_input_iterator const &rhs) const
  {
    return not(*this == rhs);
  }

private:
  void advance()
  {
    assert(m_home != nullptr);
    m_home->receive_row(m_value);
    if (m_home->done()) m_home = nullptr;
  }

  stream_t *m_home = nullptr;
  value_type m_value;
};


template<typename... TYPE> inline
auto
stream_query<TYPE...>::begin() &
{
  return stream_query_input_iterator<TYPE...>{*this};
}


template<typename... TYPE> inline
auto
stream_query<TYPE...>::end() const &
{
  return stream_query_input_iterator<TYPE...>{};
}} // namespace pqxx
#endif
