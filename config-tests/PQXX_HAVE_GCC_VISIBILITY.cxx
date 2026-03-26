// Test for gcc-style "visibility" attribute.

#if !__has_cpp_attribute(gnu::visibility)
#  error "Compiler does not support gnu::visibility attribute."
#endif

struct [[gnu::visibility("hidden")]] D
{
  D() {}
  int f() { return 0; }
};

int main()
{
  D d;
  return d.f();
}
