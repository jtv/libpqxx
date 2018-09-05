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
      internal::is_container<CONTAINER>::value
    ), void>::type
  ;
  template<typename TUPLE> auto insert(const TUPLE &)
    -> typename std::enable_if<(
      internal::is_tuple<TUPLE>::value
    ), void>::type
  ;
  
  template<typename IT> void push_back(IT Begin, IT End);
  template<typename CONTAINER> auto push_back(const CONTAINER &c)
    -> typename std::enable_if<(
      internal::is_container<CONTAINER>::value
    ), void>::type
  ;
  template<typename TUPLE> auto push_back(const TUPLE &)
    -> typename std::enable_if<(
      internal::is_tuple<TUPLE>::value
    ), void>::type
  ;
  
  template<typename SIZE> void reserve(SIZE) {}
  
  template<typename CONTAINER> auto operator<<(const CONTAINER &c)
    -> typename std::enable_if<(
      internal::is_container<CONTAINER>::value
    ), tablewriter &>::type
  ;
  tablewriter &operator<<(tablereader &);
  template<typename TUPLE> auto operator<<(const TUPLE &)
    -> typename std::enable_if<(
      internal::is_tuple<TUPLE>::value
    ), tablewriter &>::type
  ;
  
  template<typename IT> std::string generate(IT Begin, IT End) const;
  template<typename CONTAINER> auto generate(const CONTAINER &c) const
    -> typename std::enable_if<(
      internal::is_container<CONTAINER>::value
    ), std::string>::type
  ;
  template<typename TUPLE> auto generate(const TUPLE &)
    -> typename std::enable_if<(
      internal::is_tuple<TUPLE>::value
    ), std::string>::type
  ;
  
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
PQXX_LIBEXPORT std::string escape(const std::string &s);

template<typename T> inline std::string escape_any(const T &t)
{
  return escape(to_string(t));
}
template<> inline std::string escape_any<std::string>(const std::string &s)
{
  return escape(s);
}


class IteratorEscaper
{
  const std::string &m_null;
public:
  explicit IteratorEscaper(const std::string &null) : m_null(null) {}
  
  // Can't use a simple template specialization for these as we don't know ahead
  // of time what iterator types could return `const char*`
  template<typename IT> auto operator()(IT i) const
    -> typename std::enable_if<
      !std::is_same<decltype(*i), const char*>::value,
      std::string
    >::type
  {
    return to_string(*i) == m_null ? "\\N" : escape_any(*i);
  }
  template<typename IT> auto operator()(IT i) const
    -> typename std::enable_if<
      std::is_same<decltype(*i), const char*>::value,
      std::string
    >::type
  {
    return i ? escape_any(std::string{i}) : "\\N";
  }
};

class TypedEscaper
{
public:
  template<typename T> std::string operator()(const T* t) const
  {
    return pqxx::string_traits<T>::is_null(*t) ? "\\N" : escape(to_string(*t));
  }
};
// Explicit specialization so we don't need a string_traits<> for nullptr_t
template<> inline std::string TypedEscaper::operator()<std::nullptr_t>(
  const std::nullptr_t*
) const
  { return "\\N"; }
}


template<typename IT>
inline std::string tablewriter::generate(IT Begin, IT End) const
{
  return separated_list("\t", Begin, End, internal::IteratorEscaper(NullStr()));
}

template<typename CONTAINER>
inline auto tablewriter::generate(const CONTAINER &c) const
  -> typename std::enable_if<(
    internal::is_container<CONTAINER>::value
  ), std::string>::type
{
  return generate(std::begin(c), std::end(c));
}

template<typename TUPLE> auto tablewriter::generate(const TUPLE &t)
  -> typename std::enable_if<(
    internal::is_tuple<TUPLE>::value
  ), std::string>::type
{
  return separated_list("\t", t, internal::TypedEscaper());
}

template<typename IT> inline void tablewriter::insert(IT Begin, IT End)
{
  write_raw_line(generate(Begin, End));
}

template<typename CONTAINER> inline auto tablewriter::insert(const CONTAINER &c)
  -> typename std::enable_if<(
    internal::is_container<CONTAINER>::value
  ), void>::type
{
  insert(std::begin(c), std::end(c));
}

template<typename TUPLE> auto tablewriter::insert(const TUPLE &t)
  -> typename std::enable_if<(
    internal::is_tuple<TUPLE>::value
  ), void>::type
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
    internal::is_container<CONTAINER>::value
  ), void>::type
{
  insert(std::begin(c), std::end(c));
}

template<typename TUPLE> auto tablewriter::push_back(const TUPLE &t)
  -> typename std::enable_if<(
    internal::is_tuple<TUPLE>::value
  ), void>::type
{
  insert(t);
}

template<typename CONTAINER>
inline auto tablewriter::operator<<(const CONTAINER &c)
  -> typename std::enable_if<(
    internal::is_container<CONTAINER>::value
  ), tablewriter &>::type
{
  insert(c);
  return *this;
}

template<typename TUPLE> auto tablewriter::operator<<(const TUPLE &t)
  -> typename std::enable_if<(
    internal::is_tuple<TUPLE>::value
  ), tablewriter &>::type
{
  insert(t);
  return *this;
}

} // namespace pqxx
#include "pqxx/compiler-internal-post.hxx"
#endif
