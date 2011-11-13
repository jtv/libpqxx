/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablereader.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::tablereader class.
 *   pqxx::tablereader enables optimized batch reads from a database table
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablereader instead.
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_TABLEREADER
#define PQXX_H_TABLEREADER
#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/result"
#include "pqxx/tablestream"
namespace pqxx
{
/// @deprecated Efficiently pull data directly out of a table.
/** @warning This class does not work reliably with multibyte encodings.  Using
 * it with some multi-byte encodings may pose a security risk.
 */
class PQXX_LIBEXPORT tablereader : public tablestream
{
public:
  tablereader(transaction_base &,
      const PGSTD::string &Name,
      const PGSTD::string &Null=PGSTD::string());
  template<typename ITER>
  tablereader(transaction_base &,
      const PGSTD::string &Name,
      ITER begincolumns,
      ITER endcolumns);
  template<typename ITER> tablereader(transaction_base &,
      const PGSTD::string &Name,
      ITER begincolumns,
      ITER endcolumns,
      const PGSTD::string &Null);
  ~tablereader() throw ();
  template<typename TUPLE> tablereader &operator>>(TUPLE &);
  operator bool() const throw () { return !m_Done; }
  bool operator!() const throw () { return m_Done; }
  bool get_raw_line(PGSTD::string &Line);
  template<typename TUPLE>
  void tokenize(PGSTD::string, TUPLE &) const;
  virtual void complete();
private:
  void setup(transaction_base &T,
      const PGSTD::string &RName,
      const PGSTD::string &Columns=PGSTD::string());
  void PQXX_PRIVATE reader_close();
  PGSTD::string extract_field(const PGSTD::string &,
      PGSTD::string::size_type &) const;
  bool m_Done;
};
template<typename ITER> inline
tablereader::tablereader(transaction_base &T,
    const PGSTD::string &Name,
    ITER begincolumns,
    ITER endcolumns) :
  namedclass(Name, "tablereader"),
  tablestream(T, PGSTD::string()),
  m_Done(true)
{
  setup(T, Name, columnlist(begincolumns, endcolumns));
}
template<typename ITER> inline
tablereader::tablereader(transaction_base &T,
    const PGSTD::string &Name,
    ITER begincolumns,
    ITER endcolumns,
    const PGSTD::string &Null) :
  namedclass(Name, "tablereader"),
  tablestream(T, Null),
  m_Done(true)
{
  setup(T, Name, columnlist(begincolumns, endcolumns));
}
template<typename TUPLE>
inline void tablereader::tokenize(PGSTD::string Line, TUPLE &T) const
{
  PGSTD::back_insert_iterator<TUPLE> ins = PGSTD::back_inserter(T);
  PGSTD::string::size_type here=0;
  while (here < Line.size()) *ins++ = extract_field(Line, here);
}
template<typename TUPLE>
inline tablereader &pqxx::tablereader::operator>>(TUPLE &T)
{
  PGSTD::string Line;
  if (get_raw_line(Line)) tokenize(Line, T);
  return *this;
}
} // namespace pqxx
#include "pqxx/compiler-internal-post.hxx"
#endif
