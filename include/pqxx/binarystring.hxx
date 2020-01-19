/* Representation for raw, binary data.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/binarystring instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_BINARYSTRING
#define PQXX_H_BINARYSTRING

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <memory>
#include <string>
#include <string_view>

#include "pqxx/result.hxx"

namespace pqxx
{
/// Binary data corresponding to PostgreSQL's "BYTEA" binary-string type.
/** @ingroup escaping-functions
 *
 * This class represents a binary string as stored in a field of type bytea.
 * The raw value returned by a bytea field contains escape sequences for
 * certain characters, which are filtered out by binarystring.
 *
 * Internally a binarystring is zero-terminated, but it may also contain null
 * bytes, they're just like any other byte value.  So don't assume that it's
 * safe to treat the contents as a C-style string unless you've made sure of it
 * yourself.
 *
 * The binarystring retains its value even if the result it was obtained from
 * is destroyed, but it cannot be copied or assigned.
 *
 * \relatesalso transaction_base::esc_raw
 *
 * To convert the other way, i.e. from a raw series of bytes to a string
 * suitable for inclusion as bytea values in your SQL, use the transaction's
 * esc_raw() functions.
 *
 * @warning This class is implemented as a reference-counting smart pointer.
 * Copying, swapping, and destroying binarystring objects that refer to the
 * same underlying data block is <em>not thread-safe</em>.  If you wish to pass
 * binarystrings around between threads, make sure that each of these
 * operations is protected against concurrency with similar operations on the
 * same object, or other objects pointing to the same data block.
 */
class PQXX_LIBEXPORT binarystring
{
public:
  using char_type = unsigned char;
  using value_type = std::char_traits<char_type>::char_type;
  using size_type = size_t;
  using difference_type = long;
  using const_reference = value_type const &;
  using const_pointer = value_type const *;
  using const_iterator = const_pointer;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  binarystring(binarystring const &) = default;

  /// Read and unescape bytea field.
  /** The field will be zero-terminated, even if the original bytea field
   * isn't.
   * @param F the field to read; must be a bytea field
   */
  explicit binarystring(field const &);

  /// Copy binary data from std::string_view.
  explicit binarystring(std::string_view);

  /// Copy binary data of given length straight out of memory.
  binarystring(void const *, size_t);

  /// Size of converted string in bytes.
  [[nodiscard]] size_type size() const noexcept { return m_size; }
  /// Size of converted string in bytes.
  [[nodiscard]] size_type length() const noexcept { return size(); }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  [[nodiscard]] const_iterator begin() const noexcept { return data(); }
  [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return data() + m_size; }
  [[nodiscard]] const_iterator cend() const noexcept { return end(); }

  [[nodiscard]] const_reference front() const noexcept { return *begin(); }
  [[nodiscard]] const_reference back() const noexcept
  {
    return *(data() + m_size - 1);
  }

  [[nodiscard]] const_reverse_iterator rbegin() const
  {
    return const_reverse_iterator{end()};
  }
  [[nodiscard]] const_reverse_iterator crbegin() const { return rbegin(); }
  [[nodiscard]] const_reverse_iterator rend() const
  {
    return const_reverse_iterator{begin()};
  }
  [[nodiscard]] const_reverse_iterator crend() const { return rend(); }

  /// Unescaped field contents.
  [[nodiscard]] value_type const *data() const noexcept { return m_buf.get(); }

  [[nodiscard]] const_reference operator[](size_type i) const noexcept
  {
    return data()[i];
  }

  [[nodiscard]] PQXX_PURE bool operator==(binarystring const &) const noexcept;
  [[nodiscard]] bool operator!=(binarystring const &rhs) const noexcept
  {
    return not operator==(rhs);
  }

  binarystring &operator=(binarystring const &);

  /// Index contained string, checking for valid index.
  const_reference at(size_type) const;

  /// Swap contents with other binarystring.
  void swap(binarystring &);

  /// Raw character buffer (no terminating zero is added).
  /** @warning No terminating zero is added!  If the binary data did not end in
   * a null character, you will not find one here.
   */
  [[nodiscard]] char const *get() const noexcept
  {
    return reinterpret_cast<char const *>(m_buf.get());
  }

  /// Read contents as a std::string_view.
  [[nodiscard]] std::string_view view() const noexcept
  {
    return std::string_view(get(), size());
  }

  /// Read as regular C++ string (may include null characters).
  /** This creates and returns a new string object.  Don't call this
   * repeatedly; retrieve your string once and keep it in a local variable.
   * Also, do not expect to be able to compare the string's address to that of
   * an earlier invocation.
   */
  [[nodiscard]] std::string str() const;

private:
  std::shared_ptr<value_type> m_buf;
  size_type m_size{0};
};
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
