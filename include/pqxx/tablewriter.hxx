/** Definition of the pqxx::tablewriter class.
 *
 * pqxx::tablewriter enables optimized batch updates to a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablewriter.hxx instead.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TABLEWRITER
#define PQXX_H_TABLEWRITER

#include <iterator>
#include <type_traits>

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/tablestream.hxx"


namespace pqxx
{
/// @deprecated Efficiently write data directly to a database table.
/** @warning This class does not work reliably with multibyte encodings.  Using
 * it with some multi-byte encodings may pose a security risk.
 */
class PQXX_LIBEXPORT tablewriter : public tablestream
{
public:
  tablewriter(
	transaction_base &,
	const std::string &WName,
	const std::string &Null=std::string());
  template<typename ITER> tablewriter(
	transaction_base &,
	const std::string &WName,
	ITER begincolumns,
	ITER endcolumns);
  template<typename ITER> tablewriter(
	transaction_base &T,
	const std::string &WName,
	ITER begincolumns,
	ITER endcolumns,
	const std::string &Null);
  ~tablewriter() noexcept;
  
  template<typename IT> void insert(IT Begin, IT End);
  template<typename CONTAINER> auto insert(const CONTAINER &c)
    -> typename std::enable_if<(
      !std::is_void<decltype(std::begin(c))>::value
      && !std::is_void<decltype(std::end(c))>::value
    ), void>::type
  ;
  template<
    typename TUPLE,
    std::size_t INDEX=0,
    typename std::enable_if<
      (INDEX <= std::tuple_size<TUPLE>::value),
      int
    >::type=0
  > void insert(const TUPLE &);
  
  template<typename IT> void push_back(IT Begin, IT End);
  template<typename CONTAINER> auto push_back(const CONTAINER &c)
    -> typename std::enable_if<(
      !std::is_void<decltype(std::begin(c))>::value
      && !std::is_void<decltype(std::end(c))>::value
    ), void>::type
  ;
  template<
    typename TUPLE,
    std::size_t INDEX=0,
    typename std::enable_if<
      (INDEX <= std::tuple_size<TUPLE>::value),
      int
    >::type=0
  > void push_back(const TUPLE &);
  
  template<typename SIZE> void reserve(SIZE) {}
  
  template<typename CONTAINER> auto operator<<(const CONTAINER &c)
    -> typename std::enable_if<(
      !std::is_void<decltype(std::begin(c))>::value
      && !std::is_void<decltype(std::end(c))>::value
    ), tablewriter &>::type
  ;
  tablewriter &operator<<(tablereader &);
  template<
    typename TUPLE,
    std::size_t INDEX=0,
    typename std::enable_if<
      (INDEX <= std::tuple_size<TUPLE>::value),
      int
    >::type=0
  > tablewriter &operator<<(const TUPLE &);
  
  template<typename IT> std::string generate(IT Begin, IT End) const;
  template<typename CONTAINER> auto generate(const CONTAINER &c) const
    -> typename std::enable_if<(
      !std::is_void<decltype(std::begin(c))>::value
      && !std::is_void<decltype(std::end(c))>::value
    ), std::string>::type
  ;
  template<
    typename TUPLE,
    std::size_t INDEX=0,
    typename std::enable_if<
      (INDEX <= std::tuple_size<TUPLE>::value),
      int
    >::type=0
  > std::string generate(const TUPLE &);
  
  virtual void complete() override;
  void write_raw_line(const std::string &);
private:
  void setup(
	transaction_base &,
	const std::string &WName,
	const std::string &Columns = std::string());
  PQXX_PRIVATE void writer_close();
};
} // namespace pqxx


namespace std
{
template<>
  class back_insert_iterator<pqxx::tablewriter> :
	public iterator<output_iterator_tag, void,void,void,void>
{
public:
  explicit back_insert_iterator(pqxx::tablewriter &W) noexcept :
    m_writer(&W) {}

  back_insert_iterator &
    operator=(const back_insert_iterator &rhs) noexcept
  {
    m_writer = rhs.m_writer;
    return *this;
  }

  template<typename CONTAINER>
  back_insert_iterator &operator=(const CONTAINER &c)
  {
    m_writer->insert(c);
    return *this;
  }

  back_insert_iterator &operator++() { return *this; }
  back_insert_iterator &operator++(int) { return *this; }
  back_insert_iterator &operator*() { return *this; }

private:
  pqxx::tablewriter *m_writer;
};
} // namespace std


namespace pqxx
{
template<typename ITER> inline tablewriter::tablewriter(
	transaction_base &T,
	const std::string &WName,
	ITER begincolumns,
	ITER endcolumns) :
  namedclass("tablewriter", WName),
  tablestream(T, std::string())
{
  setup(T, WName, columnlist(begincolumns, endcolumns));
}


template<typename ITER> inline tablewriter::tablewriter(
	transaction_base &T,
	const std::string &WName,
	ITER begincolumns,
	ITER endcolumns,
	const std::string &Null) :
  namedclass("tablewriter", WName),
  tablestream(T, Null)
{
  setup(T, WName, columnlist(begincolumns, endcolumns));
}


namespace internal
{
PQXX_LIBEXPORT std::string escape(
	const std::string &s,
	const std::string &null);

inline std::string escape_any(
	const std::string &s,
	const std::string &null)
{ return escape(s, null); }

inline std::string escape_any(
	const char s[],
	const std::string &null)
{ return s ? escape(std::string(s), null) : "\\N"; }

template<typename T> inline std::string escape_any(
	const T &t,
	const std::string &null)
{ return escape(to_string(t), null); }


class Escaper
{
  const std::string &m_null;
public:
  explicit Escaper(const std::string &null) : m_null(null) {}
  template<typename IT> std::string operator()(IT i) const
    { return escape_any(*i, m_null); }
};
}


template<typename IT>
inline std::string tablewriter::generate(IT Begin, IT End) const
{
  return separated_list("\t", Begin, End, internal::Escaper(NullStr()));
}

template<typename CONTAINER>
inline auto tablewriter::generate(const CONTAINER &c) const
  -> typename std::enable_if<(
    !std::is_void<decltype(std::begin(c))>::value
    && !std::is_void<decltype(std::end(c))>::value
  ), std::string>::type
{
  return generate(std::begin(c), std::end(c));
}

template<
  typename TUPLE,
  std::size_t INDEX,
  typename std::enable_if<
    (INDEX <= std::tuple_size<TUPLE>::value),
    int
  >::type
> std::string tablewriter::generate(const TUPLE &t)
{
  return separated_list("\t", t, internal::Escaper(NullStr()));
}

template<typename IT> inline void tablewriter::insert(IT Begin, IT End)
{
  write_raw_line(generate(Begin, End));
}

template<typename CONTAINER> inline auto tablewriter::insert(const CONTAINER &c)
  -> typename std::enable_if<(
    !std::is_void<decltype(std::begin(c))>::value
    && !std::is_void<decltype(std::end(c))>::value
  ), void>::type
{
  insert(std::begin(c), std::end(c));
}

template<
  typename TUPLE,
  std::size_t INDEX,
  typename std::enable_if<
    (INDEX <= std::tuple_size<TUPLE>::value),
    int
  >::type
> void tablewriter::insert(const TUPLE &t)
{
  write_raw_line(generate(t));
}

template<typename IT>
inline void tablewriter::push_back(IT Begin, IT End)
{
  insert(Begin, End);
}

template<typename CONTAINER>
inline auto tablewriter::push_back(const CONTAINER &c)
  -> typename std::enable_if<(
    !std::is_void<decltype(std::begin(c))>::value
    && !std::is_void<decltype(std::end(c))>::value
  ), void>::type
{
  insert(std::begin(c), std::end(c));
}

template<
  typename TUPLE,
  std::size_t INDEX,
  typename std::enable_if<
    (INDEX <= std::tuple_size<TUPLE>::value),
    int
  >::type
> void tablewriter::push_back(const TUPLE &t)
{
  insert(t);
}

template<typename CONTAINER>
inline auto tablewriter::operator<<(const CONTAINER &c)
  -> typename std::enable_if<(
    !std::is_void<decltype(std::begin(c))>::value
    && !std::is_void<decltype(std::end(c))>::value
  ), tablewriter &>::type
{
  insert(c);
  return *this;
}

template<
  typename TUPLE,
  std::size_t INDEX,
  typename std::enable_if<
    (INDEX <= std::tuple_size<TUPLE>::value),
    int
  >::type
> tablewriter &tablewriter::operator<<(const TUPLE &t)
{
  insert(t);
  return *this;
}

} // namespace pqxx
#include "pqxx/compiler-internal-post.hxx"
#endif
