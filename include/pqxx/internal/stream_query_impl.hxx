/* Code for parts of pqxx::internal::stream_query.
 *
 * These definitions need to be in a separate file in order to iron out
 * circular dependencies between headers.
 */
#if !defined(PQXX_H_STREAM_QUERY_IMPL)
#  define PQXX_H_STREAM_QUERY_IMPL

namespace pqxx::internal
{
template<typename... TYPE>
inline stream_query<TYPE...>::stream_query(
  transaction_base &tx, std::string_view query, conversion_context c) :
        transaction_focus{tx, "stream_query"},
        m_char_finder{get_finder(tx, c.loc)},
        m_ctx{std::move(c)}
{
  auto const r{tx.exec(std::format("COPY ({}) TO STDOUT", query), m_ctx.loc)};
  r.expect_columns(sizeof...(TYPE), m_ctx.loc);
  r.expect_rows(0, m_ctx.loc);
  register_me();
}


template<typename... TYPE>
PQXX_RETURNS_NONNULL inline char_finder_func *
stream_query<TYPE...>::get_finder(transaction_base const &tx, sl loc)
{
  auto const group{tx.conn().get_encoding_group(loc)};
  return get_char_finder<'\t', '\\'>(group, loc);
}


// TODO: Replace with generator?  Could be faster (local vars vs. members).
/// Minimal iterator for stream_query.
/** Just barely enough to support range-based "for" loops on @ref stream_query.
 * It's so minimal, it isn't even an `input_iterator.
 *
 * Do not assume that anything beyond that works: post-increment, comparison to
 * anything other than `end()`, assignment between iterators on different
 * streams, and probably several more common and sensible things to do with
 * iterators are all anathema here.
 */
template<typename... TYPE> class stream_query_iterator final
{
  using stream_t = stream_query<TYPE...>;

public:
  using value_type = std::tuple<TYPE...>;
  using difference_type = long;

  stream_query_iterator(stream_t &home, sl loc) :
          m_home(&home),
          m_line{typename stream_query<TYPE...>::line_handle(
            nullptr, pqxx::internal::pq::pqfreemem)},
          m_created_loc{loc}
  {
    consume_line(loc);
  }

  stream_query_iterator(stream_query_iterator const &) = delete;
  stream_query_iterator &operator=(stream_query_iterator const &) = delete;
  stream_query_iterator(stream_query_iterator &&) = delete;
  stream_query_iterator &operator=(stream_query_iterator &&) = delete;

  /// Pre-increment.
  /** We don't even support post-increment, because we only do what's needed
   * for range-based `for` loops.
   */
  stream_query_iterator &operator++() &
  {
    assert(not done());
    consume_line(m_created_loc);
    return *this;
  }

  /// Dereference.  There's no caching in here, so don't repeat calls.
  value_type operator*() const
  {
    return m_home->parse_line(std::string_view{m_line.get(), m_line_size});
  }

  /// Are we at the end?
  bool operator==(stream_query_end_iterator) const noexcept { return done(); }

  /// Do we have more iterations to go?
  bool operator!=(stream_query_end_iterator) const noexcept
  {
    return not done();
  }

private:
  /// Have we finished?
  bool done() const noexcept { return m_home->done(); }

  /// Read a line from the stream, store it in the iterator.
  /** Replaces the newline at the end with a tab, as a sentinel to simplify
   * (and thus hopefully speed up) the field parsing loop.
   */
  void consume_line(sl loc) &
  {
    auto [line, size]{m_home->read_line(loc)};
    m_line = std::move(line);
    m_line_size = size;
    if (m_line)
    {
      // We know how many fields to expect.  Replace the newline at the end
      // with the field separator, so the parsing loop only needs to scan for a
      // tab, not a tab or a newline.
      char *const ptr{m_line.get()};
      assert(ptr[size] == '\n');
      ptr[size] = '\t';
    }
  }

  stream_t *const m_home = nullptr;

  /// Last COPY line we read, allocated by libpq.
  typename stream_t::line_handle m_line;

  /// Length of the last COPY line we read.
  std::size_t m_line_size;

  /// A `std::source_location` for where this object was created.
  sl const m_created_loc;
};


template<typename... TYPE>
PQXX_INLINE_ONLY inline bool
operator==(stream_query_end_iterator, stream_query_iterator<TYPE...> const &i)
{
  return i.done();
}


template<typename... TYPE>
PQXX_INLINE_ONLY inline bool
operator!=(stream_query_end_iterator, stream_query_iterator<TYPE...> const &i)
{
  return not i.done();
}


template<typename... TYPE> inline auto stream_query<TYPE...>::begin() &
{
  return stream_query_iterator<TYPE...>{*this, m_ctx.loc};
}


template<typename... TYPE>
inline std::pair<typename stream_query<TYPE...>::line_handle, std::size_t>
stream_query<TYPE...>::read_line(sl loc) &
{
  assert(not done());

  internal::gate::connection_stream_from gate{m_trans->conn()};
  try
  {
    auto line{gate.read_copy_line(loc)};
    if (not line.first) [[unlikely]]
    {
      // This is how we get told the iteration is finished.
      close();
    }
    return line;
  }
  catch (std::exception const &)
  {
    close();
    throw;
  }
}
} // namespace pqxx::internal
#endif
