#ifndef PQXX_H_CALLGATE
#define PQXX_H_CALLGATE

namespace pqxx
{
namespace internal
{
/// Base class for call gates.
/**
 * A call gate defines a limited, private interface on the host class that
 * specified client classes can access.
 *
 * The metaphor works as follows: the gate stands in front of a "home," which is
 * really a class, and only lets specific friends in.
 *
 * To implement a call gate that gives client C access to host H,
 *  - derive a gate class from callgate<H>;
 *  - make the gate class a friend of H;
 *  - make C a friend of the gate class; and
 *  - implement "stuff C can do with H" inside the gate class.
 *
 * This special kind of "gated" friendship gives C private access to H, but only
 * through an expressly limited interface. 
 */
template<typename HOME> class PQXX_PRIVATE callgate
{
protected:
  /// This class, to keep constructors easy.
  typedef callgate<HOME> super;
  /// A reference to the host class.  Helps keep constructors easy.
  typedef HOME &reference;

  callgate(reference x) : m_home(x) {}

  /// The home object.  The gate class has full "private" access.
  const reference home() const throw () { return m_home; }
  /// The home object.  The gate class has full "private" access.
  reference home() throw () { return m_home; }

private:
  reference m_home;
};
} // namespace pqxx::internal
} // namespace pqxx

#endif
