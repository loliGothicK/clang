// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out
#define USE_PRINTF 1

#ifndef USE_PRINTF
# define printf(...) 

#else

# if USE_PRINTF 
#   include <cstdio>
# else
#   define printf(...) 
# endif

#endif



struct X { };
template<class T> X foo(T);
char foo(int);

int main() {
//* This compiles
{
  int n = 10;
  auto L = [](auto n2) [](auto a) -> decltype(n) a;
  auto M = L(5);
  auto N = M(3.14);
}
//*/

//* Referring to previous parameter - with capture
{
  int n = 10;
  auto L = [](auto n2) [=](auto a) -> decltype(n2) a + n2;
  auto M = L(5);
  auto N = M(3.14);
}
//*/
//* No capture, referring to parameter
{
  auto L = [](auto n2) [](auto a) -> decltype(foo(n2)) a;
  auto M = L(5);
  char c = M(62.14);
  //CHECK: M(62.14) = >
  printf("M(62.14) = %c\n", M(62.14));
  
}
//*/

//* This compiles
{
  auto L = [](auto n2) [](auto a) [](auto b) -> decltype(foo(n2)) b;
  auto M = L(5);
  auto N = M(62.14);
  auto O = N(63.157);
  //CHECK-NEXT: N(63.157) = ?
  printf("N(63.157) = %c\n", N(63.157));
  
}
//*/
}