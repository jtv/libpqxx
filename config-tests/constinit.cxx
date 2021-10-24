// Test for constinit.
namespace
{
constinit int x{0};
}


int main()
{
  return x;
}
