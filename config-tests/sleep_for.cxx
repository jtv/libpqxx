// Test for std::this_thread::sleep_for().
#include <chrono>
#include <thread>

int main()
{
  std::this_thread::sleep_for(std::chrono::microseconds{10u});
}
