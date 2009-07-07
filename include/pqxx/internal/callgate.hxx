#ifndef PQXX_H_CALLGATE
#define PQXX_H_CALLGATE

namespace pqxx
{
namespace internal
{
template<typename HOME> class PQXX_PRIVATE callgate
{
protected:
  typedef callgate<HOME> super;
  typedef HOME &reference;

  callgate(reference x) : m_home(x) {}
  const reference home() const throw () { return m_home; }
  reference home() throw () { return m_home; }

private:
  reference m_home;
};
} // namespace pqxx::internal
} // namespace pqxx

#endif
