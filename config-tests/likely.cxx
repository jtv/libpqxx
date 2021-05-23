// Test for C++20 [[likely]] and [[unlikely]] attributes.

int main(int argc, char **)
{
  if (argc == 1) [[likely]] return 0;
  else [[unlikely]] return 1;
}
