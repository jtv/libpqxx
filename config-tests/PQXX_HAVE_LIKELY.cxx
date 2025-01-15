// Test for C++20 [[likely]] and [[unlikely]] attributes.
// C++20: Assume support.

#if !__has_cpp_attribute(likely)
#  error "No support for [[likely]] / [[unlikely]] attributes."
#endif

int foo(int i)
{
  if (i > 0) [[likely]]
    return 100;
  else
    return 0;
}
