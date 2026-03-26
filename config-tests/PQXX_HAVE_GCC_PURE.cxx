// Test for gcc-style "pure" attribute.
[[gnu::pure]] int f()
{
  return 0;
}

int main()
{
  return f();
}
