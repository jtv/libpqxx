// Test for gcc-style "visibility" attribute.
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
