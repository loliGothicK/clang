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
//*
template<int N> struct X {
  using fptr = X (*)(int);
  static fptr div;	
  X(int) { };
};

//int (*fp) (int) = [](auto a) a;
template<int N> 
typename X<N>::fptr X<N>::div = [](auto a) X<N>(a + N);
//*/
//*
template<int N> struct Y {
  static int A;
};
template<int N> 
int Y<N>::A = ([](auto a) N + a)(3);


int main() 
{
 //CHECK: Y<4>::A = 7
 printf("Y<4>::A = %d\n", Y<4>::A);

}
