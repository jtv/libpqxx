#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_field_ref final : callgate<result const>
{
  friend class pqxx::field_ref;

  constexpr result_field_ref(reference x) : super(x) {}

  [[nodiscard]] PQXX_PURE char const *
  get_value(result_size_type row_num, row_size_type col_num) const noexcept
  {
    return home().get_value(row_num, col_num);
  }

  [[nodiscard]] PQXX_PURE bool
  get_is_null(result_size_type row_num, row_size_type col_num) const noexcept
  {
    return home().get_is_null(row_num, col_num);
  }

  [[nodiscard]] PQXX_PURE field_size_type
  get_length(result_size_type row_num, row_size_type col_num) const noexcept
  {
    return home().get_length(row_num, col_num);
  }

  [[nodiscard]] oid column_type(row_size_type col_num, sl loc) const
  {
    return home().column_type(col_num, loc);
  }

  [[nodiscard]] oid column_table(row_size_type col_num, sl loc) const
  {
    return home().column_table(col_num, loc);
  }

  [[nodiscard]] encoding_group encoding() const noexcept
  {
    return home().m_encoding;
  }
};
} // namespace pqxx::internal::gate
