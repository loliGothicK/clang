// RUN: %clang -std=c++1y -c %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif


int main() {
//*
{
  auto L1 = [](auto a) -> decltype(a) 5;
  //CHECK: L1('c') = 5
  printf("L1('c') = %d\n", L1('c'));
}
//*/
//*
{
  int local = 10;
  auto L = [=](auto a) mutable ++local + a(2.0);
  //auto N = L(([=](auto a) mutable -> decltype(a) local+=a), 3.0);
  auto M = L([=](auto a) mutable -> decltype(a) local+=a);
  //CHECK: M = 23.000000
  printf("M = %f\n", M);
}
//*/  
//*
{
  int local = 10;
  //auto L = [=](auto a, auto b) mutable ++local + a(b);
  auto L = [=](auto a, auto b) mutable ++local + a(b);
  auto N = [=](auto a) a;
  auto M = L(([=](auto a) mutable -> decltype(a) local+=a), 4.0);
  //auto M = L(N);
  //auto M = L([=](auto a) mutable -> decltype(a) { return local+=a; }, 3.0);
  //auto M = L(([=](auto a) mutable -> decltype(a) local+=a), 3.0);
  //auto M = L([=](auto a) mutable -> decltype(a) local+=a);
  //CHECK: M = 25.000000
  printf("M = %f\n", M);
}
//*/
/*
{
  int local = 10;
  auto L = [=](auto a) mutable ++local + a(2.0);
  //auto M = L(([=](auto a) mutable -> decltype(a) local+=a), 3.0);
  auto M = L([=](auto a) mutable ->decltype(a) local+=a);
  printf("M = %f\n", M);
}
//*/
/*
{
  //int local = 10;
  auto L = [](auto a) a(2.0);
  L([](auto a) ->decltype(a) 1);
  //auto M = L(([=](auto a) mutable -> decltype(a) local+=a), 3.0);
  //auto M = L([=](auto a) mutable ->decltype(a) local+=a);
  //printf("M = %f\n", M);
}
//*/

}
