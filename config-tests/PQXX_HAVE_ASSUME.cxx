int main(int argc, char **argv)
{
  [[assume(argv != nullptr)]];
  return argc - 1;
}
