#include <libpq-fe.h>

int main()
{
  return PQsocketPoll(0, 1, 0, 2'000'000'000);
}
