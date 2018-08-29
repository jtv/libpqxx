/** Definition of the pqxx::tablereader class.
 *
 * pqxx::tablereader enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablereader instead.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TABLEREADER
#define PQXX_H_TABLEREADER

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/except.hxx"
#include "pqxx/internal/type_utils.hxx"
#include "pqxx/result.hxx"
#include "pqxx/tablestream.hxx"


namespace pqxx
{
/// @deprecated Efficiently pull data directly out of a table.
/** @warning This class does not work reliably with multibyte encodings.  Using
 * it with some multi-byte encodings may pose a security risk.
 */
class PQXX_LIBEXPORT tablereader : public tablestream
{
public:
  tablereader(
	transaction_base &,
	const std::string &Name,
	const std::string &Null=std::string());
  template<typename ITER>
  tablereader(
	transaction_base &,
	const std::string &Name,
	ITER begincolumns,
	ITER endcolumns);
  template<typename ITER> tablereader(
	transaction_base &,
	const std::string &Name,
	ITER begincolumns,
	ITER endcolumns,
	const std::string &Null);
  ~tablereader() noexcept;
  template<typename CONTAINER> auto operator>>(CONTAINER &c)
    -> typename std::enable_if<(
      !std::is_void<decltype(std::begin(c))>::value
      && !std::is_void<decltype(std::end(c))>::value
    ), tablereader &>::type
  ;
  template< typename TUPLE > auto operator>>(TUPLE &)
    -> typename std::enable_if<(
      std::tuple_size<TUPLE>::value >= 0
    ), tablereader &>::type
  ;
  operator bool() const noexcept { return !m_done; }
  bool operator!() const noexcept { return m_done; }
  bool get_raw_line(std::string &Line);
  template<typename CONTAINER>
  auto tokenize(const std::string &, CONTAINER &c) const
    -> typename std::enable_if<(
      !std::is_void<decltype(std::begin(c))>::value
      && !std::is_void<decltype(std::end(c))>::value
    ), void>::type
  ;
  template<typename TUPLE>
  auto tokenize(const std::string &, TUPLE &) const
    -> typename std::enable_if<(
      std::tuple_size<TUPLE>::value >= 0
    ), void>::type
  ;
  virtual void complete() override;
private:
  void setup(
	transaction_base &T,
	const std::string &RName,
	const std::string &Columns=std::string());
  PQXX_PRIVATE void reader_close();
  std::string extract_field(
	const std::string &,
	std::string::size_type &) const;
  bool extract_field(
	const std::string &,
	std::string::size_type &,
	std::string &) const;
  bool m_done;
  internal::encoding_group m_copy_encoding;
  
  template<typename TUPLE, std::size_t I> auto tokenize_ith(
    const std::string &,
    TUPLE &,
    std::string::size_type,
    std::string &
  ) const -> typename std::enable_if<(
      std::tuple_size<TUPLE>::value > I
    ), void>::type
  ;
  template<typename TUPLE, std::size_t I> auto tokenize_ith(
    const std::string &,
    TUPLE &,
    std::string::size_type,
    std::string &
  ) const -> typename std::enable_if<(
      std::tuple_size<TUPLE>::value <= I
    ), void>::type
  ;
  
  template<typename T> auto extract_value(
    const std::string &Line,
    T& t,
    std::string::size_type &here,
    std::string &workspace
  ) const -> typename std::enable_if<
    internal::is_derefable<T>::value,
    void
  >::type;
  template<typename T> auto extract_value(
    const std::string &Line,
    T& t,
    std::string::size_type &here,
    std::string &workspace
  ) const -> typename std::enable_if<
    !internal::is_derefable<T>::value,
    void
  >::type;
};


template<typename ITER> inline
tablereader::tablereader(
	transaction_base &T,
	const std::string &Name,
	ITER begincolumns,
	ITER endcolumns) :
  namedclass(Name, "tablereader"),
  tablestream(T, std::string()),
  m_done(true)
{
  setup(T, Name, columnlist(begincolumns, endcolumns));
}


template<typename ITER> inline
tablereader::tablereader(
	transaction_base &T,
	const std::string &Name,
	ITER begincolumns,
	ITER endcolumns,
	const std::string &Null) :
  namedclass(Name, "tablereader"),
  tablestream(T, Null),
  m_done(true)
{
  setup(T, Name, columnlist(begincolumns, endcolumns));
}


template<typename CONTAINER>
inline auto tablereader::tokenize(const std::string &Line, CONTAINER &c) const
  -> typename std::enable_if<(
    !std::is_void<decltype(std::begin(c))>::value
    && !std::is_void<decltype(std::end(c))>::value
  ), void>::type
{
  std::back_insert_iterator<CONTAINER> ins = std::back_inserter(c);
  std::string::size_type here=0;
  while (here < Line.size()) *ins++ = extract_field(Line, here);
}

template<typename TUPLE>
inline auto tablereader::tokenize(const std::string &Line, TUPLE &t) const
  -> typename std::enable_if<(
    std::tuple_size<TUPLE>::value >= 0
  ), void>::type
{
  std::string workspace;
  tokenize_ith<TUPLE, 0>(Line, t, 0, workspace);
}

template<typename TUPLE, std::size_t I> auto tablereader::tokenize_ith(
  const std::string &Line,
  TUPLE &t,
  std::string::size_type here,
  std::string &workspace
) const -> typename std::enable_if<(
    std::tuple_size<TUPLE>::value > I
  ), void>::type
{
  if (here < Line.size())
  {
    extract_value(Line, std::get<I>(t), here, workspace);
    tokenize_ith<TUPLE, I+1>(Line, t, here, workspace);
  }
  else
    throw pqxx::usage_error{"Too few fields to extract from tablereader line"};
}

template<typename TUPLE, std::size_t I> auto tablereader::tokenize_ith(
  const std::string &Line,
  TUPLE &t,
  std::string::size_type here,
  std::string &
) const -> typename std::enable_if<(
    std::tuple_size<TUPLE>::value <= I
  ), void>::type
{
  // Zero-column line may still have a trailing newline
  if (here < Line.size() && !(here == Line.size() - 1 && Line[here] == '\n'))
    throw pqxx::usage_error{"Not all fields extracted from tablereader line"};
}


template<typename T> auto tablereader::extract_value(
  const std::string &Line,
  T& t,
  std::string::size_type &here,
  std::string &workspace
) const -> typename std::enable_if<
  internal::is_derefable<T>::value,
  void
>::type
{
  if (extract_field(Line, here, workspace))
    from_string<internal::inner_type<T>>(workspace, *t);
  else
    t = internal::null_value<T>();
}
template<typename T> auto tablereader::extract_value(
  const std::string &Line,
  T& t,
  std::string::size_type &here,
  std::string &workspace
) const -> typename std::enable_if<
  !internal::is_derefable<T>::value,
  void
>::type
{
  if (extract_field(Line, here, workspace))
    from_string<T>(workspace, t);
  else
    t = internal::null_value<T>();
}


template<typename CONTAINER>
inline auto pqxx::tablereader::operator>>(CONTAINER &c)
  -> typename std::enable_if<(
    !std::is_void<decltype(std::begin(c))>::value
    && !std::is_void<decltype(std::end(c))>::value
  ), tablereader &>::type
{
  std::string Line;
  if (get_raw_line(Line)) tokenize(Line, c);
  return *this;
}

template< typename TUPLE >
inline auto tablereader::operator>>(TUPLE &t)
  -> typename std::enable_if<(
    std::tuple_size<TUPLE>::value >= 0
  ), tablereader &>::type
{
  std::string Line;
  if (get_raw_line(Line)) tokenize(Line, t);
  return *this;
}
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"
#endif
