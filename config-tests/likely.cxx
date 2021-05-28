// Test for C++20 [[likely]] and [[unlikely]] attributes.

int main(int argc, char **)
{
  int x = 0;
  if (argc == 1) [[likely]]
    x = 0;
  else
    x = 1;
  return x;
}
