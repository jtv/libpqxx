// Feature check for 'PQXX_HAVE_CONCEPTS'.
// Generated by generate_cxx_checks.py.
#include <version>
#if !defined(__cpp_concepts) || !__cpp_concepts
#error "No support for PQXX_HAVE_CONCEPTS detected: __cpp_concepts not set."
#endif

int main() {}
