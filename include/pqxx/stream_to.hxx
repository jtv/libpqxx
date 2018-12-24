/** Definition of the pqxx::stream_to class.
 *
 * pqxx::stream_to enables optimized batch updates to a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_to.hxx instead.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_STREAM_TO
#define PQXX_H_STREAM_TO

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/stream_base.hxx"
#include "pqxx/stream_from.hxx"
#include "pqxx/internal/type_utils.hxx"

#include <string>


namespace pqxx
{

/// Efficiently write data directly to a database table.
class PQXX_LIBEXPORT stream_to : public stream_base
{
public:
  stream_to(
    transaction_base &,
    const std::string &table_name
  );
  template<typename Columns> stream_to(
    transaction_base &,
    const std::string &table_name,
    const Columns& columns
  );
  template<typename Iter> stream_to(
    transaction_base &,
    const std::string &table_name,
    Iter columns_begin,
    Iter columns_end
  );

  ~stream_to() noexcept;

  void complete() override;

  void write_raw_line(const std::string &);
  template<typename Tuple> stream_to & operator<<(const Tuple &);

  /// Stream a `stream_from` straight into a `stream_to`.
  /** This can be useful when copying between different databases.  If the
   * source and the destination are on the same database, you'll get better
   * performance doing it all in a regular query.
   */
  stream_to &operator<<(stream_from &);

private:
  void set_up(transaction_base &, const std::string &table_name);
  void set_up(
    transaction_base &,
    const std::string &table_name,
    const std::string &columns
  );

  void close() override;
};


template<typename Columns> stream_to::stream_to(
  transaction_base &tb,
  const std::string &table_name,
  const Columns& columns
) : stream_to{
  tb,
  table_name,
  std::begin(columns),
  std::end(columns)
}
{}


template<typename Iter> stream_to::stream_to(
  transaction_base &tb,
  const std::string &table_name,
  Iter columns_begin,
  Iter columns_end
) :
  namedclass{"stream_from", table_name},
  stream_base{tb}
{
  set_up(
    tb,
    table_name,
    columnlist(columns_begin, columns_end)
  );
}


namespace internal
{

class PQXX_LIBEXPORT TypedCopyEscaper
{
  static std::string escape(const std::string &);
public:
  template<typename T> std::string operator()(const T* t) const
  {
    return string_traits<T>::is_null(*t) ? "\\N" : escape(to_string(*t));
  }
};

// Explicit specialization so we don't need a string_traits<> for nullptr_t
template<> inline std::string TypedCopyEscaper::operator()<std::nullptr_t>(
  const std::nullptr_t*
) const
{ return "\\N"; }

} // namespace pqxx::internal


template<typename Tuple> stream_to & stream_to::operator<<(const Tuple &t)
{
  write_raw_line(separated_list("\t", t, internal::TypedCopyEscaper()));
  return *this;
}

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif
