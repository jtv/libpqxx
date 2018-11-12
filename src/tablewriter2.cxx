/** Implementation of the pqxx::tablewriter2 class.
 *
 * pqxx::tablewriter2 enables optimized batch updates to a database table.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/tablewriter2"

#include "pqxx/internal/gates/transaction-tablewriter2.hxx"


pqxx::tablewriter2::tablewriter2(
  transaction_base &tb,
  const std::string &table_name
) :
  namedclass("tablereader2", table_name),
  tablestream2(tb)
{
  setup(tb, table_name);
}


pqxx::tablewriter2::~tablewriter2() noexcept
{
  try
  {
    complete();
  }
  catch (const std::exception &e)
  {
    reg_pending_error(e.what());
  }
}


void pqxx::tablewriter2::complete()
{
  close();
}


void pqxx::tablewriter2::write_raw_line(const std::string &line)
{
  internal::gate::transaction_tablewriter2{m_trans}.write_copy_line(line);
}


pqxx::tablewriter2 & pqxx::tablewriter2::operator<<(tablereader2 &tr)
{
  std::string line;
  while (tr)
  {
    tr.get_raw_line(line);
    write_raw_line(line);
  }
  return *this;
}


void pqxx::tablewriter2::setup(
  transaction_base &tb,
  const std::string &table_name
)
{
  setup(tb, table_name, "");
}


void pqxx::tablewriter2::setup(
  transaction_base &tb,
  const std::string &table_name,
  const std::string &columns
)
{
  internal::gate::transaction_tablewriter2{tb}.BeginCopyWrite(
    table_name,
    columns
  );
  register_me();
}


void pqxx::tablewriter2::close()
{
  if (*this)
  {
    tablestream2::close();
    try
    {
      internal::gate::transaction_tablewriter2{m_trans}.end_copy_write();
    }
    catch (const std::exception &)
    {
      try
      {
        tablestream2::close();
      }
      catch (const std::exception &) {}
      throw;
    }
  }
}


std::string pqxx::internal::TypedCopyEscaper::escape(const std::string &s)
{
  if (s.empty())
    return s;

  std::string escaped;
  escaped.reserve(s.size()+1);

  for (auto c : s)
    switch (c)
    {
    case '\b': escaped += "\\b";  break; // Backspace
    case '\f': escaped += "\\f";  break; // Vertical tab
    case '\n': escaped += "\\n";  break; // Form feed
    case '\r': escaped += "\\r";  break; // Newline
    case '\t': escaped += "\\t";  break; // Tab
    case '\v': escaped += "\\v";  break; // Carriage return
    case '\\': escaped += "\\\\"; break; // Backslash
    default:
      if (c < ' ' || c > '~')
      {
        escaped += "\\";
        for (auto i = 2; i >= 0; --i)
          escaped += number_to_digit((c >> (3*i)) & 0x07);
      }
      else
        escaped += c;
      break;
    }

  return escaped;
}
