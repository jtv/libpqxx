/* Definitions for the pqxx::field class.
 *
 * pqxx::field refers to a field in a query result.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/field instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_FIELD
#define PQXX_H_FIELD

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <optional>

#include "pqxx/array.hxx"
#include "pqxx/composite.hxx"
#include "pqxx/internal/gates/result-field_ref.hxx"
#include "pqxx/result.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/types.hxx"

namespace pqxx::internal::gate
{
class field_ref_const_row_iterator;
} // namespace pqxx::internal::gate


namespace pqxx
{
/// Lightweight reference to a field in a result set.
/** Like @ref field, represents one field in a query result set.  Unlike with
 * @ref field, however, for as long as you're using a `field_ref`, the
 * @ref result object must...
 *
 * 1. remain valid, i.e. you can't destroyit;
 * 2. and in the same place in memory, i.e. you can't move it;
 * 3. and keep the same value, i.e you can't assign to it.
 *
 * When you use `field_ref`, it is your responsibility to ensure all that.
 *
 * You can query whether a `field_ref` is null, and if not, you can convert its
 * value from its textual "SQL representation" to a more suitable C++ type.
 */
class PQXX_LIBEXPORT field_ref final
{
public:
  /// A type for holding the number of bytes in a field.
  using size_type = field_size_type;

  field_ref() noexcept = default;
  field_ref(field_ref const &) noexcept = default;
  field_ref(
    result const &res, result_size_type row_num,
    row_size_type col_num) noexcept :
          m_result(&res), m_row{row_num}, m_column{col_num}
  {}

  field_ref &operator=(field_ref const &) noexcept = default;

  [[nodiscard]] PQXX_PURE result const &home() const noexcept
  {
    return *m_result;
  }
  [[nodiscard]] PQXX_PURE result_size_type row_number() const noexcept
  {
    return m_row;
  }

  /**
   * @name Comparison
   *
   * Equality between two `field_ref` objects means that they both refer to
   * the same row and column in _the exact same @ref result object._
   *
   * So, if you copy a @ref result, even though the two copies refer to the
   * exact same underlying data structure, a `field_ref` in the one will never
   * be equal to a `field_ref` in the other.
   */
  //@{
  [[nodiscard]] PQXX_PURE bool operator==(field_ref const &rhs) const noexcept
  {
    return (home() == rhs.home()) and (row_number() == rhs.row_number()) and
           (column_number() == rhs.column_number());
  }
  [[nodiscard]] PQXX_PURE bool operator!=(field_ref const &rhs) const noexcept
  {
    return not operator==(rhs);
  }
  //@}

  /**
   * @name Column information
   */
  //@{
  /// Column name.
  [[nodiscard]] PQXX_PURE char const *name(sl loc = sl::current()) const &
  {
    return home().column_name(column_number(), loc);
  }

  /// Column type.
  [[nodiscard]] PQXX_PURE oid type(sl loc = sl::current()) const;

  /// What table did this column come from?
  [[nodiscard]] PQXX_PURE oid table(sl = sl::current()) const;

  /// Return column number.  The first column is 0, the second is 1, etc.
  [[nodiscard]] PQXX_PURE constexpr row_size_type
  column_number() const noexcept
  {
    return m_column;
  }

  /// What column number in its originating table did this column come from?
  [[nodiscard]] PQXX_PURE row_size_type
  table_column(sl loc = sl::current()) const
  {
    return home().table_column(column_number(), loc);
  }
  //@}

  /**
   * @name Content access
   *
   * You can read a `field_ref` as any C++ type for which a conversion from
   * PostgreSQL's text format is defined.  See @ref datatypes for how this
   * works.  This mechanism is _weakly typed:_ the conversions do not care
   * what SQL type a field had in the database, only that its actual contents
   * convert to the target type without problems.  So for instance, you can
   * read a `text` field as an `int`, so long as the string in the field spells
   * out a valid `int` number.
   *
   * Many built-in types come with conversions predefined.  To find out how to
   * add your own, see @ref datatypes.
   */
  //@{
  /// Read as @ref zview, or an empty one if null.
  /** A @ref zview is also a `std::string_view`.  It just adds the promise that
   * there is a terminating zero right behind the string.
   */
  [[nodiscard]] PQXX_PURE std::string_view view() const & noexcept
  {
    return zview{c_str(), size()};
  }

  /// Read as plain C string.
  /** Since the field's data is stored internally in the form of a
   * zero-terminated C string, this is the fastest way to read it.  Use the
   * @ref is_null() and @ref as() functions to convert the string to other
   * types such as `int`, or to C++ strings.
   *
   * @warning Binary data may contain null bytes, so do not use `c_str()` for
   * those.  Instead, convert the value to a binary type using @ref as(), e.g.
   * `f.as<pqxx::bytes>()`.
   */
  [[nodiscard]] PQXX_PURE char const *c_str() const & noexcept;

  /// Is this field's value null?
  [[nodiscard]] PQXX_PURE bool is_null() const noexcept;

  /// Return number of bytes taken up by the field's value.
  [[nodiscard]] PQXX_PURE size_type size() const noexcept;

  /// Return value as object of given type, or `default value` if null.
  /** Note that unless the function is instantiated with an explicit template
   * argument, the Default value's type also determines the result type.
   */
  template<typename T>
  [[nodiscard]] T as(T const &default_value, sl loc = sl::current()) const
  {
    if (is_null())
      return default_value;
    else
      return from_string<T>(this->view(), make_context(loc));
  }

  /// Return value as object of given type, or throw exception if null.
  /** Use as `as<std::optional<int>>()` or `as<my_untemplated_optional_t>()` as
   * an alternative to `get<int>()`; this is disabled for use with raw pointers
   * (other than C-strings) because storage for the value can't safely be
   * allocated here
   */
  template<typename T> [[nodiscard]] T as(sl loc = sl::current()) const
  {
    if (is_null())
    {
      if constexpr (not has_null<T>())
        internal::throw_null_conversion(name_type<T>(), loc);
      else
        return make_null<T>();
    }
    else
    {
      return from_string<T>(this->view(), make_context(loc));
    }
  }

  /// Read value into `obj`; or if null, leave `obj` untouched.
  /** This can be handy to read a field's value but also check for nullness
   * along the way.
   *
   * @return Whether the field contained an actual value.  So: `true` for a
   * non-null field, or `false` for a null field.
   */
  template<typename T> bool to(T &obj, sl loc = sl::current()) const
  {
    if (is_null())
    {
      return false;
    }
    else
    {
      from_string(view(), obj, make_context(loc));
      return true;
    }
  }

  /// Read value into `obj`; or if null, set default value and return `false`.
  template<typename T>
  bool to(T &obj, T const &default_value, sl loc = sl::current()) const
  {
    bool const null{is_null()};
    if (null)
      obj = default_value;
    else
      obj = from_string<T>(this->view(), make_context(loc));
    return not null;
  }

  /// Return value wrapped in some optional type (empty for nulls).
  /** Use as `get<int>()` as before to obtain previous behavior, or specify
   * container type with `get<int, std::optional>()`
   */
  template<typename T, template<typename> typename O = std::optional>
  constexpr O<T> get() const
  {
    return as<O<T>>();
  }

  /// Read SQL array contents as a @ref pqxx::array.
  template<typename ELEMENT, auto... ARGS>
  [[deprecated("Use as<pqxx::array<ELEMENT, ...>>().")]]
  array<ELEMENT, ARGS...> as_sql_array(sl loc = sl::current()) const
  {
    using array_type = array<ELEMENT, ARGS...>;

    // There's no such thing as a null SQL array.
    if (is_null())
      internal::throw_null_conversion(name_type<array_type>(), loc);
    else
      return array_type{
        this->view(),
        pqxx::internal::gate::result_field_ref{home()}.encoding(), loc};
  }
  //@}

private:
  friend class pqxx::internal::gate::field_ref_const_row_iterator;

  /// Jump n columns ahead (negative to jump back).
  void offset(row_difference_type n) { m_column += n; }

  /// Build a @ref conversion_context, using the result's encoding group.
  conversion_context make_context(sl loc) const
  {
    return conversion_context{home().get_encoding_group(), loc};
  }

  /// The result in which we're iterating.  Must remain valid.
  result const *m_result = nullptr;

  /// Row's number inside the result.
  result_size_type m_row = -1;

  /// Field's column number inside the result.
  /**
   * You'd expect this to be unsigned, but due to the way reverse iterators
   * are related to regular iterators, it must be allowed to underflow to -1.
   */
  row_size_type m_column = -1;
};
} // namespace pqxx


#include "pqxx/internal/gates/field_ref-const_row_iterator.hxx"


namespace pqxx
{
/// Reference to a field in a result set.
/** This is like @ref field_ref, except it's safe to destroy the @ref result
 * object or move it to a different place in memory.
 *
 * A field represents one entry in a row.  It represents an actual value
 * in the result set, and can be converted to various types.
 */
class PQXX_LIBEXPORT field final
{
public:
  using size_type = field_size_type;

  field(field_ref const &f) :
          m_home{f.home()}, m_row{f.row_number()}, m_col{f.column_number()}
  {}

  /**
   * @name Comparison
   */
  //@{
  /// Byte-by-byte comparison of two fields (all nulls are considered equal)
  /** @warning This differs from what comparisons do in @ref result, @ref row,
   * @ref row_ref, @ref field, @ref field_ref, and the iterator classes.  It
   * will change in the future to compare only the fields' identities, not the
   * actual data.
   *
   * Handling of null values differs from that in SQL where a comparison
   * involving a null value yields null, so nulls are never considered equal
   * to one another or even to themselves.
   *
   * Null handling also probably differs from the closest equivalent in C++,
   * which is the NaN (Not-a-Number) value, a singularity comparable to
   * SQL's null.  This is because the builtin == operator demands that a == a.
   *
   * The usefulness of this operator is questionable.  No interpretation
   * whatsoever is imposed on the data; 0 and 0.0 are considered different,
   * as are null vs. the empty string, or even different (but possibly
   * equivalent and equally valid) encodings of the same Unicode character
   * etc.
   */
  [[deprecated(
    "To compare fields by content, compare their view()s.")]] PQXX_PURE bool
  operator==(field const &) const noexcept;

  /// Byte-by-byte comparison (all nulls are considered equal)
  /** @warning See operator==() for important information about this operator
   */
  [[deprecated(
    "To compare fields by content, compare their view()s.")]] PQXX_PURE bool
  operator!=(field const &rhs) const noexcept
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return not operator==(rhs);
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }
  //@}

  /**
   * @name Column information
   */
  //@{
  /// Column name.
  [[nodiscard]] PQXX_PURE char const *name(sl = sl::current()) const &;

  /// Column type.
  [[nodiscard]] PQXX_PURE oid type(sl loc = sl::current()) const
  {
    return as_field_ref().type(loc);
  }

  /// What table did this column come from?
  [[nodiscard]] PQXX_PURE oid table(sl loc = sl::current()) const
  {
    return as_field_ref().table(loc);
  }

  /// Return column number.  The first column is 0, the second is 1, etc.
  [[deprecated("Use column_number().")]] row_size_type num() const noexcept
  {
    return column_number();
  }

  /// What column number in its originating table did this column come from?
  [[nodiscard]] row_size_type table_column(sl = sl::current()) const;
  //@}

  /**
   * @name Content access
   *
   * You can read a field as any C++ type for which a conversion from
   * PostgreSQL's text format is defined.  See @ref datatypes for how this
   * works.  This mechanism is _weakly typed:_ the conversions do not care
   * what SQL type a field had in the database, only that its actual contents
   * convert to the target type without problems.  So for instance, you can
   * read a `text` field as an `int`, so long as the string in the field spells
   * out a valid `int` number.
   *
   * Many built-in types come with conversions predefined.  To find out how to
   * add your own, see @ref datatypes.
   */
  //@{
  /// Read as @ref zview (which is also a `string_view`).
  /** Returns an empty view if the field is null.
   *
   * The result only remains usable while the data for the underlying
   * @ref result exists.  Once all `result` objects referring to that data have
   * been destroyed, the view will no longer point to valid memory.
   */
  [[nodiscard]] PQXX_PURE std::string_view view() const & noexcept
  {
    return {c_str(), size()};
  }

  /// Read as plain C string.
  /** Since the field's data is stored internally in the form of a
   * zero-terminated C string, this is the fastest way to read it.  Use the
   * `to()` or `as()` functions to convert the string to other types such as
   * `int`, or to C++ strings.
   *
   * Do not use `c_str()` for BYTEA values, or other binary values.  To read
   * those, convert the value to some binary type using `to()` or `as()`.  For
   * example: `f.as<pqx::bytes>()`.
   *
   * If the field is null, returns an empty string.
   */
  [[nodiscard]] PQXX_PURE PQXX_RETURNS_NONNULL char const *
  c_str() const & noexcept
  {
    return as_field_ref().c_str();
  }

  /// Is this field's value null?
  [[nodiscard]] PQXX_PURE bool is_null() const noexcept
  {
    return as_field_ref().is_null();
  }

  /// Return number of bytes taken up by the field's value.
  [[nodiscard]] PQXX_PURE size_type size() const noexcept
  {
    return as_field_ref().size();
  }

  /// Read value into `obj`; or if null, leave `obj` untouched.
  /** This can be handy to read a field's value but also check for nullness
   * along the way.
   *
   * @return Whether the field contained an actual value.  So: `true` for a
   * non-null field, or `false` for a null field.
   */
  template<typename T> bool to(T &obj, sl loc = sl::current()) const
  {
    if (is_null())
    {
      return false;
    }
    else
    {
      obj = from_string<T>(view(), make_context(loc));
      return true;
    }
  }

  /// Read field as a composite value, write its components into `fields`.
  /** Returns whether the field was null.  If it was, it will not touch the
   * values in `fields`.
   */
  template<typename... T> bool composite_to(sl loc, T &...fields) const
  {
    if (is_null())
    {
      return false;
    }
    else
    {
      parse_composite(make_context(loc), view(), fields...);
      return true;
    }
  }

  /// Read field as a composite value, write its components into `fields`.
  /** @warning This version does not accept a `std::source_location` (what in
   * libpqxx we call @ref pqxx::sl for short).  That means that should
   * conversion fail with an exception, the exception will refer to this
   * function.  It's generally more helpful to pass a location that's more
   * meaningful in the context of your codebase, using the version of this
   * function that takes it as a first argumet.
   *
   * Returns whether the field was null.  If it was, it will not touch the
   * values in `fields`.
   */
  template<typename... T> bool composite_to(T &...fields) const
  {
    return composite_to(sl::current(), fields...);
  }

  /// Read value into obj; or leave obj untouched and return `false` if null.
  template<typename T>
  [[deprecated("Use to() or as().")]] bool operator>>(T &obj) const
  {
    return to(obj);
  }

  /// Read value into obj; or if null, use default value and return `false`.
  /** This can be used with `std::optional`, as well as with standard smart
   * pointer types, but not with raw pointers.
   */
  template<typename T>
  bool to(T &obj, T const &default_value, sl loc = sl::current()) const
  {
    return as_field_ref().to<T>(obj, default_value, make_context(loc));
  }

  /// Return value as object of given type, or default value if null.
  /** Note that unless the function is instantiated with an explicit template
   * argument, the Default value's type also determines the result type.
   */
  template<typename T>
  [[nodiscard]] T as(T const &default_value, sl loc = sl::current()) const
  {
    return as_field_ref().as<T>(default_value, loc);
  }

  /// Return value as object of given type, or throw exception if null.
  /** Use as `as<std::optional<int>>()` or `as<my_untemplated_optional_t>()` as
   * an alternative to `get<int>()`; this is disabled for use with raw pointers
   * (other than C-strings) because storage for the value can't safely be
   * allocated here
   */
  template<typename T> [[nodiscard]] T as(sl loc = sl::current()) const
  {
    return as_field_ref().as<T>(loc);
  }

  /// Return value wrapped in some optional type (empty for nulls).
  /** Use as `get<int>()` as before to obtain previous behavior, or specify
   * container type with `get<int, std::optional>()`
   */
  template<typename T, template<typename> typename O = std::optional>
  constexpr O<T> get() const
  {
    return as<O<T>>();
  }

  /// Read SQL array contents as a @ref pqxx::array.
  template<typename ELEMENT, auto... ARGS>
  array<ELEMENT, ARGS...> as_sql_array(sl loc = sl::current()) const
  {
    return as_field_ref().as_sql_array<ELEMENT, ARGS...>(loc);
  }

  /// Parse the field as an SQL array.
  /** Call the parser to retrieve values (and structure) from the array.
   *
   * Make sure the @ref result object stays alive until parsing is finished. If
   * you keep the @ref row of `field` object alive, it will keep the @ref
   * result object alive as well.
   */
  [[deprecated(
    "Avoid pqxx::array_parser.  "
    "Instead, use as_sql_array() to convert to pqxx::array.")]]
  array_parser as_array() const & noexcept
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return array_parser{c_str(), m_home.get_encoding_group()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }
  //@}

  /// This field's row number within the result.
  [[nodiscard]] PQXX_PURE result_size_type row_number() const noexcept
  {
    return m_row;
  }
  /// This field's column number within the result.
  [[nodiscard]] PQXX_PURE row_size_type column_number() const noexcept
  {
    return m_col;
  }

private:
  /** Create field as reference to a field in a result set.
   * @param r Row that this field is part of.
   * @param c Column number of this field.
   */
  field(row const &r, row_size_type c) noexcept;

  /// Constructor.
  field() noexcept = default;

  /// Retun @ref field_ref for this field.
  /** @warning The @ref field_ref holds a reference to the @ref result
   * object _inside this `field` object._  So if you change that, the
   * @ref field_ref becomes invalid.
   */
  field_ref as_field_ref() const noexcept
  {
    return field_ref{home(), row_number(), column_number()};
  }

  constexpr result const &home() const noexcept { return m_home; }

  field(
    result const &r, result_size_type row_num, row_size_type col_num) noexcept
          :
          m_home{r}, m_row{row_num}, m_col{col_num}
  {}

  /// Build a @ref conversion_context, using the result's encoding group.
  conversion_context make_context(sl loc) const
  {
    return conversion_context{home().get_encoding_group(), loc};
  }

  result m_home;
  result::size_type m_row;
  /**
   * You'd expect this to be unsigned, but due to the way reverse iterators
   * are related to regular iterators, it must be allowed to underflow to -1.
   */
  row_size_type m_col;
};


inline char const *field_ref::c_str() const & noexcept
{
  return pqxx::internal::gate::result_field_ref{home()}.get_value(
    row_number(), column_number());
}

inline bool field_ref::is_null() const noexcept
{
  return pqxx::internal::gate::result_field_ref{home()}.get_is_null(
    row_number(), column_number());
}

inline field_size_type field_ref::size() const noexcept
{
  return pqxx::internal::gate::result_field_ref{home()}.get_length(
    row_number(), column_number());
}

inline oid field_ref::type(sl loc) const
{
  return pqxx::internal::gate::result_field_ref{home()}.column_type(
    column_number(), loc);
}

inline oid field_ref::table(sl loc) const
{
  return pqxx::internal::gate::result_field_ref{home()}.column_table(
    column_number(), loc);
}


/// Specialization: `to(zview &)`.
/** This conversion is not generally available, since the general conversion
 * would not know whether there was indeed a terminating zero at the end of
 * the string.  (It could check, but it would have no way of knowing that a
 * zero occurring after the string in memory was actually part of the same
 * allocation.)
 */
template<> inline bool field::to<zview>(zview &obj, sl) const
{
  bool const null{is_null()};
  if (not null)
    obj = zview{c_str(), size()};
  return not null;
}


template<>
inline bool field::to<zview>(zview &obj, zview const &default_value, sl) const
{
  bool const null{is_null()};
  if (null)
    obj = default_value;
  else
    obj = zview{c_str(), size()};
  return not null;
}


/// Efficient specialisation: you can convert a field to a `zview`.
/** String conversions generally accept `std::string_view`.  You can't just
 * "convert" any old `std::string_view` to a `pqxx::zview` because `zview` is
 * a promise that the string is zero-terminated.  One can't generally make that
 * promise based on a `string_view`.
 *
 */
template<> inline zview field_ref::as<zview>(sl loc) const
{
  if (is_null())
    internal::throw_null_conversion(name_type<zview>(), loc);
  return zview{c_str(), size()};
}


template<> inline zview field::as<zview>(zview const &default_value, sl) const
{
  return is_null() ? default_value : zview{c_str(), size()};
}


template<typename CHAR = char, typename TRAITS = std::char_traits<CHAR>>
class field_streambuf final : public std::basic_streambuf<CHAR, TRAITS>
{
public:
  using char_type = CHAR;
  using traits_type = TRAITS;
  using int_type = typename traits_type::int_type;
  using pos_type = typename traits_type::pos_type;
  using off_type = typename traits_type::off_type;
  using openmode = std::ios::openmode;
  using seekdir = std::ios::seekdir;

  explicit field_streambuf(field const &f) : m_field{f} { initialize(); }

protected:
  int sync() override { return traits_type::eof(); }

  pos_type seekoff(off_type, seekdir, openmode) override
  {
    return traits_type::eof();
  }
  pos_type seekpos(pos_type, openmode) override { return traits_type::eof(); }
  int_type overflow(int_type) override { return traits_type::eof(); }
  int_type underflow() override { return traits_type::eof(); }

private:
  field const &m_field;

  int_type initialize()
  {
    auto g{static_cast<char_type *>(const_cast<char *>(m_field.c_str()))};
    this->setg(g, g, g + std::size(m_field));
    return int_type(std::size(m_field));
  }
};


/// Input stream that gets its data from a result field
/** @deprecated To convert a field's value string to some other type, e.g. to
 * an `int`, use the field's `as<...>()` member function.  To read a field
 * efficiently just as a string, use its `c_str()` or its
 * `as<std::string_vview>()`.
 *
 * Works like any other istream to read data from a field.  It supports all
 * formatting and streaming operations of `std::istream`.  For convenience
 * there is a fieldstream alias, which defines a @ref basic_fieldstream for
 * `char`.  This is similar to how e.g. `std::ifstream` relates to
 * `std::basic_ifstream`.
 *
 * This class has only been tested for the char type (and its default traits).
 */
template<typename CHAR = char, typename TRAITS = std::char_traits<CHAR>>
class basic_fieldstream final : public std::basic_istream<CHAR, TRAITS>
{
  using super = std::basic_istream<CHAR, TRAITS>;

public:
  using char_type = CHAR;
  using traits_type = TRAITS;
  using int_type = typename traits_type::int_type;
  using pos_type = typename traits_type::pos_type;
  using off_type = typename traits_type::off_type;

  [[deprecated("Use field::as<...>() or field::c_str().")]] basic_fieldstream(
    field const &f) :
          super{nullptr}, m_buf{f}
  {
    super::init(&m_buf);
  }

private:
  field_streambuf<CHAR, TRAITS> m_buf;
};


/// @deprecated Read a field using `field::as<...>()` or `field::c_str()`.
using fieldstream = basic_fieldstream<char>;


/// Write a result field to any type of stream.
/** @deprecated The C++ streams library is not great to work with.  In
 * particular, error handling is easy to get wrong.  So you're probably better
 * off doing this by hand.
 *
 * This can be convenient when writing a field to an output stream.  More
 * importantly, it lets you write a field to e.g. a `stringstream` which you
 * can then use to read, format and convert the field in ways that to() does
 * not support.
 *
 * Example: parse a field into a variable of the nonstandard `long long` type.
 *
 * ```cxx
 * extern result R;
 * long long L;
 * stringstream S;
 *
 * // Write field's string into S
 * S << R[0][0];
 *
 * // Parse contents of S into L
 * S >> L;
 * ```
 */
template<typename CHAR>
[[deprecated("To write a pqxx::field, use its view() method.")]] inline std::
  basic_ostream<CHAR> &
  operator<<(std::basic_ostream<CHAR> &s, field const &value)
{
  s.write(value.c_str(), std::streamsize(std::size(value)));
  return s;
}


/// Convert a field's value to type `T`.
/** Unlike the "regular" `from_string`, this knows how to deal with null
 * values.
 */
template<typename T> inline T from_string(field const &value, ctx c = {})
{
  if (value.is_null())
  {
    if constexpr (has_null<T>())
      return make_null<T>();
    else
      internal::throw_null_conversion(name_type<T>(), c.loc);
  }
  else
  {
    return from_string<T>(value.view(), c);
  }
}


// TODO: Can we make this generic across all "always-null" types?
// TODO: Do the same for streams.
/// Convert a field's value to `nullptr_t`.
/** Yes, you read that right.  This conversion does nothing useful.  It always
 * returns `nullptr`.
 *
 * Except... what if the field is not null?  In that case, this throws
 * @ref conversion_error.
 */
template<>
inline std::nullptr_t from_string<std::nullptr_t>(field const &value, ctx c)
{
  if (not value.is_null())
    throw conversion_error{
      "Extracting non-null field into nullptr_t variable.", c.loc};
  return nullptr;
}


/// Convert a field_ref to a string.
template<>
PQXX_LIBEXPORT inline std::string to_string(field_ref const &value, ctx)
{
  return std::string{value.view()};
}
/// Convert a field to a string.
template<> PQXX_LIBEXPORT inline std::string to_string(field const &value, ctx)
{
  return std::string{value.view()};
}
} // namespace pqxx
#endif
