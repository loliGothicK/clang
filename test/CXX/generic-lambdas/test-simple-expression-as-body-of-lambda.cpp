#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif

int main() {
{
  auto L = [](auto a) a + 3;
  int i = L(7);
  printf("i = %d\n", i);
}
//* 
{
  auto L = ([](auto a) ([=](auto b) [=](auto c) a + b + c));
  auto C = L(7);
  int i = C(10)(12);
  printf("i = %d L(7)(10)(12) = %d\n", i, L(7)(10)(12));

}
//*/
}