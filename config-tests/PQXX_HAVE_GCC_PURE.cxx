// Test for gcc-style "pure" attribute.

#if !__has_cpp_attribute(gnu::pure)
#  error "Compiler does not support gnu::pure attribute."
#endif

[[gnu::pure]] int f()
{
  return 0;
}

int main()
{
  return f();
}
