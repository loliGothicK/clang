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

template<class T> struct X { 
      T v; 
      X(T v) :v(v) { } 
      int get() const { return v; } 
};
 
int main() {
  
//* Test simple template template parameter
{
  auto L = []<template <class> class TT>(TT<int> tt) tt.get();
  int (*fp)(X<int>) = L;
  //CHECK: fp(X<int>(10)) = 10
  printf("fp(X<int>(10)) = %d\n", fp(X<int>(10)));
}
//*/

//* Test template template parameters with variadics
{
  auto L = []<template <class...> class TT>(TT<int> tt) tt.get();
  int (*fp)(X<int>) = L;
  //CHECK: fp(X<int>(11)) = 11
  printf("fp(X<int>(11)) = %d\n", fp(X<int>(11)));
}
//*/
//* Test template template parameters with more variadics
{
  auto L = []<template <class...> class TT, template<class...> class ... MoreTTs>
                  (TT<int> tt) tt.get();
  int (*fp)(X<int>) = L;
  //CHECK: fp(X<int>(12)) = 12
  printf("fp(X<int>(12)) = %d\n", fp(X<int>(12)));
}
//*/
//* Test template template parameters with even more variadics
{
  auto L = []<template<class...> class ... MoreTTs, class ... Ts>
                  (MoreTTs<Ts...> ... tt) 0;
  int (*fp)(X<int>) = L;
  //CHECK: fp(X<int>(12)) = 0
  printf("fp(X<int>(12)) = %d\n", fp(X<int>(12)));
}
//*/

}