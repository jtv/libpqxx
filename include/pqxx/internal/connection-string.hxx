#if !defined(PQXX_H_CONNECTION_STRING)
#  define PQXX_H_CONNECTION_STRING

namespace pqxx::internal
{
/// Placeholder for libpq PQconninfoOption type.
struct pg_conn_option;


/// Helper for parsing a connection string into option keys and their values.
/** This is a class and not a function for just one reason: the outputs contain
 * pointers to storage that needs to be held in memory.
 */
class PQXX_LIBEXPORT parsed_connection_string final
{
public:
  explicit parsed_connection_string(
    char const connection_string[], sl = sl::current());
  parsed_connection_string() = delete;

  ~parsed_connection_string() noexcept;

  /// Parse the string, return matching vectors of option names & values.
  /** The two vectors are of equal length.  The first holds the option names
   * and the second their respective values.
   *
   * The outputs remain valid only for as long as the whole
   * `parsed_connection_string` does.  You can call `parse()` as many times as
   * you like.  The calls will produce distinct outputs but the ultimate string
   * pointers will be the same, and in the same order.
   *
   * The vectors will only contain values that were actually specified (as
   * opposed to ones that were left to their defaults), but they will have
   * enough room reserved to specify all possible options, plus a terminating
   * null that we may need to add.
   */
  std::array<std::vector<char const *>, 2u> parse() const;

private:
  std::unique_ptr<pg_conn_option[], void (*)(pg_conn_option *)> m_options;
};
} // namespace pqxx::internal

#endif
