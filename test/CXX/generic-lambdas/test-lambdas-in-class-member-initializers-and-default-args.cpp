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

//* Test Nested Lambdas as class-member initializers
template<class T> struct Num {
  T t = T{};
  T num = ([](auto x)[=](auto y) x + y - 2)(t)(t);
};

template<> struct Num<char> {
  char fs = ' ';
  char num = ([](auto x)[=](auto y) x + y)(fs)(fs);
};

double dglobal = 3.14;
template<class T> struct Num<T*> {
  T* ps = &dglobal;
  T num = ([](auto x)[=](auto y) x + y)(*ps)(*ps);
};

Num<int> P;
Num<char> FS;
Num<double*> PS;


template<class T> struct Num2 {
  T t = T{};
  T num = ([](auto z) z)(t);
};
//*/

int main() 
{
 //* Test Nested Lambdas
  //CHECK: P.num = -2
  printf("P.num = %d\n", P.num);
  //CHECK-NEXT: FS.num = @ (64)
  printf("FS.num = %c (%d)\n", FS.num, FS.num);
  //CHECK-NEXT: PS.num = 6.280000 
  printf("PS.num = %f\n", PS.num);
//*/  

}
